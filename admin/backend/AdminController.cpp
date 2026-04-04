// Copyright (C) 2026 Paul McKinney
#include "AdminController.h"
#include "SetupWorker.h"
#include "TeardownWorker.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSysInfo>
#include <QThread>
#include <QUuid>
#include <QVariantMap>
#include <QDebug>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// QSettings group for AdminController credentials.
// The org and app name come from QCoreApplication so that --developer-profile
// in main() routes settings to the same profile-specific file as ProjectNotes.
#define SETTINGS_GROUP "sync_api"

// ---------------------------------------------------------------------------
// Logging helper – prints function name, SQL, error, and affected rows
// ---------------------------------------------------------------------------

static void logSql(const char *context, const QString &sql,
                   const QSqlQuery &q, bool ok)
{
    if (ok) {
#ifdef QT_DEBUG
        qDebug().noquote()
            << QString("[AdminController::%1] SQL OK (rows=%2): %3")
               .arg(context)
               .arg(q.numRowsAffected())
               .arg(sql.simplified());
#endif
    } else {
#ifdef QT_DEBUG
        qWarning().noquote()
            << QString("[AdminController::%1] SQL FAILED: %2\n"
                       "  driverText : %3\n"
                       "  databaseText: %4\n"
                       "  nativeCode : %5")
               .arg(context)
               .arg(sql.simplified())
               .arg(q.lastError().driverText())
               .arg(q.lastError().databaseText())
               .arg(q.lastError().nativeErrorCode());
#endif
    }
}

// ---------------------------------------------------------------------------
// Password obfuscation – XOR with a key derived from the machine unique ID.
// ---------------------------------------------------------------------------

static QByteArray obfuscationKey()
{
    QByteArray uid = QSysInfo::machineUniqueId();
    if (uid.isEmpty())
        uid = QByteArrayLiteral("SqlSyncAdmin_fallback_2024");
    QByteArray key(32, 0);
    for (int i = 0; i < 32; ++i)
        key[i] = uid[i % uid.size()];
    return key;
}

static QString obfuscate(const QString &plain)
{
    if (plain.isEmpty()) return {};
    QByteArray data = plain.toUtf8();
    const QByteArray key = obfuscationKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromLatin1(data.toBase64());
}

static QString deobfuscate(const QString &obfuscated)
{
    if (obfuscated.isEmpty()) return {};
    QByteArray data = QByteArray::fromBase64(obfuscated.toLatin1());
    const QByteArray key = obfuscationKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromUtf8(data);
}

// ---------------------------------------------------------------------------
// Username validation – must be a valid PostgreSQL role name
// ---------------------------------------------------------------------------

bool AdminController::isValidUsername(const QString &name)
{
    static const QRegularExpression re(QStringLiteral("^[a-z_][a-z0-9_]{0,62}$"));
    const bool valid = re.match(name).hasMatch();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::isValidUsername]" << name << "->" << (valid ? "VALID" : "INVALID");
#endif
    return valid;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

AdminController::AdminController(QObject *parent)
    : QObject(parent)
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController] constructor";
#endif
    loadConnectionSettings();
}

AdminController::~AdminController()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    if (m_teardownThread) {
        m_teardownThread->quit();
        m_teardownThread->wait();
    }
}

// ---------------------------------------------------------------------------
// QSettings – connection credentials
// ---------------------------------------------------------------------------

void AdminController::loadConnectionSettings()
{
    QSettings s(QCoreApplication::organizationName(), QStringLiteral("AppSettings"));
    s.beginGroup(QStringLiteral(SETTINGS_GROUP));
    setHost      (s.value(QStringLiteral("host"),       m_host).toString());
    setPort      (s.value(QStringLiteral("port"),       m_port).toInt());
    setSuperuser (s.value(QStringLiteral("superuser"),  m_superuser).toString());
    setSuperPass (deobfuscate(s.value(QStringLiteral("superPass")).toString()));
    setServerMode(s.value(QStringLiteral("syncHostType"), 0).toInt());
    setDbName    (s.value(QStringLiteral("dbName"),     m_dbName).toString());
    setSupabaseUrl       (s.value(QStringLiteral("supabaseUrl")).toString());
    setSupabaseServiceKey(deobfuscate(s.value(QStringLiteral("supabaseServiceKey")).toString()));
    s.endGroup();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::loadConnectionSettings]"
             << "host=" << m_host << "port=" << m_port
             << "db=" << m_dbName << "user=" << m_superuser
             << "serverMode=" << static_cast<int>(m_serverMode)
             << "pass set:" << !m_superPass.isEmpty();
#endif
}

void AdminController::saveConnectionSettings()
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController::saveConnectionSettings]"
             << "host=" << m_host << "port=" << m_port
             << "user=" << m_superuser
             << "serverMode=" << static_cast<int>(m_serverMode);
#endif
    QSettings s(QCoreApplication::organizationName(), QStringLiteral("AppSettings"));
    s.beginGroup(QStringLiteral(SETTINGS_GROUP));
    s.setValue(QStringLiteral("host"),                    m_host);
    s.setValue(QStringLiteral("port"),                    m_port);
    s.setValue(QStringLiteral("superuser"),               m_superuser);
    s.setValue(QStringLiteral("superPass"),               obfuscate(m_superPass));
    s.setValue(QStringLiteral("syncHostType"),             static_cast<int>(m_serverMode));
    s.setValue(QStringLiteral("dbName"),                  m_dbName);
    s.setValue(QStringLiteral("supabaseUrl"),        m_supabaseUrl);
    s.setValue(QStringLiteral("supabaseServiceKey"), obfuscate(m_supabaseServiceKey));
    s.endGroup();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

static QString randomBase64Url(int bytes = 32)
{
    QByteArray buf(bytes, Qt::Uninitialized);
    QRandomGenerator::securelySeeded().fillRange(
        reinterpret_cast<quint32 *>(buf.data()),
        (bytes + 3) / 4);
    const QByteArray b64 = buf.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return QString::fromLatin1(b64.left(bytes * 4 / 3 + 1));
}

void AdminController::generateSecrets()
{
    m_authenticatorPassword = randomBase64Url(32);
    m_jwtSecret             = randomBase64Url(48);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::generateSecrets] secrets generated";
#endif
    emit secretsReady();
}

// ---------------------------------------------------------------------------
// Private helper – create the target database if it does not yet exist.
// Connects to the "postgres" maintenance database (always present), then
// issues CREATE DATABASE if needed, then disconnects.
// ---------------------------------------------------------------------------

bool AdminController::ensureDatabaseExists(QString &errorOut)
{
    const QString connName = QStringLiteral("SqlSyncAdmin_ensure_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

#ifdef QT_DEBUG
    qDebug() << "[AdminController::ensureDatabaseExists] checking for database:" << m_dbName;
#endif

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connName);
    db.setHostName(m_host);
    db.setPort(m_port);
    db.setDatabaseName(QStringLiteral("postgres")); // maintenance DB — always exists
    db.setUserName(m_superuser);
    db.setPassword(m_superPass);

    auto cleanup = [&]() {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    };

    if (!db.open()) {
        errorOut = db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[AdminController::ensureDatabaseExists] OPEN FAILED:" << errorOut;
#endif
        cleanup();
        return false;
    }

    QSqlQuery q(db);

    // Check existence
    const QString sqlCheck = QStringLiteral(
        "SELECT 1 FROM pg_database WHERE datname = '%1'").arg(m_dbName);
    bool exists = false;
    if (q.exec(sqlCheck) && q.next()) {
        exists = true;
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::ensureDatabaseExists]" << m_dbName
             << (exists ? "already exists" : "does not exist — will create");
#endif

    if (!exists) {
        // CREATE DATABASE cannot run inside a transaction block — use exec() directly.
        const QString sqlCreate = QStringLiteral("CREATE DATABASE \"%1\"").arg(m_dbName);
#ifdef QT_DEBUG
        qDebug() << "[AdminController::ensureDatabaseExists]" << sqlCreate;
#endif
        if (!q.exec(sqlCreate)) {
            errorOut = q.lastError().databaseText();
#ifdef QT_DEBUG
            qWarning() << "[AdminController::ensureDatabaseExists] CREATE DATABASE FAILED:"
                       << errorOut;
#endif
            cleanup();
            return false;
        }
#ifdef QT_DEBUG
        qDebug() << "[AdminController::ensureDatabaseExists] database created successfully";
#endif
    }

    cleanup();
    return true;
}

QSqlDatabase AdminController::openAdminConnection() const
{
    const QString connName = QStringLiteral("SqlSyncAdmin_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
#ifdef QT_DEBUG
    qDebug() << "[AdminController::openAdminConnection]"
             << "host=" << m_host << "port=" << m_port
             << "db=" << m_dbName << "user=" << m_superuser
             << "connName=" << connName;
#endif
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connName);
    db.setHostName(m_host);
    db.setPort(m_port);
    db.setDatabaseName(m_dbName);
    db.setUserName(m_superuser);
    db.setPassword(m_superPass);
    return db;
}

void AdminController::closeAdminConnection(QSqlDatabase &db) const
{
    const QString name = db.connectionName();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::closeAdminConnection] closing" << name;
#endif
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

// ---------------------------------------------------------------------------
// Private – Better Auth HTTP helper
// ---------------------------------------------------------------------------

QJsonDocument AdminController::callSupabase(const QString       &method,
                                             const QString       &path,
                                             const QJsonDocument &body,
                                             int                 *statusOut) const
{
    const QString url = m_supabaseUrl.trimmed() + path;
#ifdef QT_DEBUG
    qDebug() << "[AdminController::callSupabase]" << method << url;
#endif

    QNetworkAccessManager nam;
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
    // Supabase admin API requires both apikey header and Bearer token set to the service role key
    req.setRawHeader("apikey", m_supabaseServiceKey.toUtf8());
    req.setRawHeader("Authorization",
                     QByteArrayLiteral("Bearer ") + m_supabaseServiceKey.toUtf8());

    QNetworkReply *reply = nullptr;
    if (method.compare(QStringLiteral("GET"), Qt::CaseInsensitive) == 0) {
        reply = nam.get(req);
    } else if (method.compare(QStringLiteral("DELETE"), Qt::CaseInsensitive) == 0) {
        reply = nam.deleteResource(req);
    } else if (method.compare(QStringLiteral("PUT"), Qt::CaseInsensitive) == 0) {
        const QByteArray payload = body.isNull() ? QByteArray() : body.toJson(QJsonDocument::Compact);
        reply = nam.put(req, payload);
    } else {
        // POST
        const QByteArray payload = body.isNull() ? QByteArray() : body.toJson(QJsonDocument::Compact);
        reply = nam.post(req, payload);
    }

    // Block until finished (synchronous pattern matching HttpClient's QEventLoop approach)
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusOut)
        *statusOut = statusCode;

    const QByteArray responseData = reply->readAll();
    reply->deleteLater();

#ifdef QT_DEBUG
    qDebug() << "[AdminController::callSupabase] status=" << statusCode
             << "response length=" << responseData.size();
#endif

    if (responseData.isEmpty())
        return QJsonDocument();

    return QJsonDocument::fromJson(responseData);
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – connection test
// ---------------------------------------------------------------------------

void AdminController::testConnection()
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController::testConnection] connecting to maintenance db";
#endif
    const QString connName = QStringLiteral("SqlSyncAdmin_test_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connName);
    db.setHostName(m_host);
    db.setPort(m_port);
    db.setDatabaseName(QStringLiteral("postgres")); // test against maintenance db
    db.setUserName(m_superuser);
    db.setPassword(m_superPass);

    if (db.open()) {
#ifdef QT_DEBUG
        qDebug() << "[AdminController::testConnection] OPEN SUCCESS";
#endif
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        emit connectionTestResult(true, QStringLiteral("Connection successful."));
    } else {
        const QString err = db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[AdminController::testConnection] OPEN FAILED:" << err;
#endif
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        emit connectionTestResult(false, err);
    }
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – check if already set up
// ---------------------------------------------------------------------------

void AdminController::checkIfAlreadySetup()
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController::checkIfAlreadySetup] start, supabaseMode=" << isSupabaseMode();
#endif

    if (!isSupabaseMode()) {
        // Self-hosted: ensure the target database exists before trying to connect.
        QString dbError;
        if (!ensureDatabaseExists(dbError)) {
#ifdef QT_DEBUG
            qWarning() << "[AdminController::checkIfAlreadySetup] cannot ensure database exists:" << dbError;
#endif
            emit connectionTestResult(false,
                QStringLiteral("Could not create database '%1': %2").arg(m_dbName, dbError));
            return;
        }
    }

    QSqlDatabase db = openAdminConnection();
    if (!db.open()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::checkIfAlreadySetup] OPEN FAILED:" << db.lastError().text();
#endif
        closeAdminConnection(db);
        emit alreadySetup(false);
        return;
    }

    QSqlQuery q(db);
    bool hasSyncData = false;
    const QString sql1 =
        QStringLiteral("SELECT COUNT(*) FROM information_schema.tables "
                       "WHERE table_schema = 'public' AND table_name = 'sync_data'");
    if (q.exec(sql1) && q.next()) {
        hasSyncData = (q.value(0).toInt() == 1);
        logSql("checkIfAlreadySetup", sql1, q, true);
    } else {
        logSql("checkIfAlreadySetup", sql1, q, false);
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::checkIfAlreadySetup] hasSyncData=" << hasSyncData;
#endif

    bool ready = false;
    if (isSupabaseMode()) {
        // In Supabase mode there is no rpc_login function; just check for sync_data.
        ready = hasSyncData;
#ifdef QT_DEBUG
        qDebug() << "[AdminController::checkIfAlreadySetup] Supabase mode — ready=" << ready;
#endif
    } else {
        bool hasRpcLogin = false;
        const QString sql2 =
            QStringLiteral("SELECT COUNT(*) FROM pg_proc p "
                           "JOIN pg_namespace n ON n.oid = p.pronamespace "
                           "WHERE n.nspname = 'public' AND p.proname = 'rpc_login'");
        if (q.exec(sql2) && q.next()) {
            hasRpcLogin = (q.value(0).toInt() > 0);
            logSql("checkIfAlreadySetup", sql2, q, true);
        } else {
            logSql("checkIfAlreadySetup", sql2, q, false);
        }
#ifdef QT_DEBUG
        qDebug() << "[AdminController::checkIfAlreadySetup] hasRpcLogin=" << hasRpcLogin;
#endif
        ready = hasSyncData && hasRpcLogin;
    }

    closeAdminConnection(db);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::checkIfAlreadySetup] emitting alreadySetup(" << ready << ")";
#endif
    emit alreadySetup(ready);
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – start setup
// ---------------------------------------------------------------------------

void AdminController::startSetup()
{
    if (m_setupRunning)
        return;

    generateSecrets();

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
        m_worker       = nullptr;
    }

    m_setupRunning = true;
    m_setupDone    = false;
    emit setupStateChanged();

    m_worker       = new SetupWorker();
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_worker, &SetupWorker::stepCompleted,
            this,     &AdminController::stepCompleted);

    connect(m_worker, &SetupWorker::finished, this,
            [this](bool success, const QString &message) {
#ifdef QT_DEBUG
                qDebug() << "[AdminController::startSetup] worker finished success=" << success << message;
#endif
                m_setupRunning = false;
                m_setupDone    = success;
                emit setupStateChanged();
                emit setupFinished(success, message);
                m_workerThread->quit();
            });

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread->start();

    const QString host       = m_host;
    const int     port       = m_port;
    const QString dbName     = m_dbName;
    const QString su         = m_superuser;
    const QString suPass     = m_superPass;
    const QString authPass   = m_authenticatorPassword;
    const QString jwt        = m_jwtSecret;
    const bool    supabaseMode = isSupabaseMode();

#ifdef QT_DEBUG
    qDebug() << "[AdminController::startSetup] invoking runSetup on worker thread"
             << "supabaseMode=" << supabaseMode;
#endif
    QMetaObject::invokeMethod(m_worker, "runSetup", Qt::QueuedConnection,
        Q_ARG(QString, host),
        Q_ARG(int,     port),
        Q_ARG(QString, dbName),
        Q_ARG(QString, su),
        Q_ARG(QString, suPass),
        Q_ARG(QString, authPass),
        Q_ARG(QString, jwt),
        Q_ARG(bool,    supabaseMode));
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – cancel setup
// ---------------------------------------------------------------------------

void AdminController::cancelSetup()
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController::cancelSetup]";
#endif
    if (m_worker)
        QMetaObject::invokeMethod(m_worker, "cancel", Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// Teardown
// ---------------------------------------------------------------------------

int AdminController::teardownTotalSteps() const
{
    // 6 functions + 2 tables + (4 roles if self-hosted)
    return isSupabaseMode() ? 8 : 12;
}

void AdminController::startTeardown()
{
    if (m_teardownRunning)
        return;

    if (m_teardownThread) {
        m_teardownThread->quit();
        m_teardownThread->wait();
        m_teardownThread->deleteLater();
        m_teardownThread  = nullptr;
        m_teardownWorker  = nullptr;
    }

    m_teardownRunning = true;
    m_teardownDone    = false;
    emit teardownStateChanged();

    m_teardownWorker = new TeardownWorker();
    m_teardownThread = new QThread(this);
    m_teardownWorker->moveToThread(m_teardownThread);

    connect(m_teardownWorker, &TeardownWorker::stepCompleted,
            this,             &AdminController::teardownStepCompleted);

    connect(m_teardownWorker, &TeardownWorker::finished, this,
            [this](bool success, const QString &message) {
#ifdef QT_DEBUG
                qDebug() << "[AdminController::startTeardown] finished success=" << success;
#endif
                m_teardownRunning = false;
                m_teardownDone    = success;
                emit teardownStateChanged();
                emit teardownFinished(success, message);
                m_teardownThread->quit();
            });

    connect(m_teardownThread, &QThread::finished,
            m_teardownWorker, &QObject::deleteLater);
    m_teardownThread->start();

#ifdef QT_DEBUG
    qDebug() << "[AdminController::startTeardown] invoking runTeardown"
             << "supabaseMode=" << isSupabaseMode();
#endif

    QMetaObject::invokeMethod(m_teardownWorker, "runTeardown", Qt::QueuedConnection,
        Q_ARG(QString, m_host),
        Q_ARG(int,     m_port),
        Q_ARG(QString, m_dbName),
        Q_ARG(QString, m_superuser),
        Q_ARG(QString, m_superPass),
        Q_ARG(bool,    isSupabaseMode()));
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – user management
// ---------------------------------------------------------------------------

void AdminController::loadUsers()
{
    if (isSupabaseMode()) {
#ifdef QT_DEBUG
        qDebug() << "[AdminController::loadUsers] Supabase mode — calling Supabase Auth";
#endif

        int status = 0;
        const QJsonDocument resp = callSupabase(
            QStringLiteral("GET"),
            QStringLiteral("/auth/v1/admin/users"),
            QJsonDocument(),
            &status);

        m_users.clear();

        if (status >= 200 && status < 300 && resp.isObject()) {
            const QJsonArray users = resp.object().value(QStringLiteral("users")).toArray();
            for (const QJsonValue &v : users) {
                const QJsonObject u = v.toObject();
                QVariantMap row;
                row[QStringLiteral("username")]   = u.value(QStringLiteral("email")).toString();
                row[QStringLiteral("id")]         = u.value(QStringLiteral("id")).toString();
                // created_at is an ISO 8601 string from Supabase
                const QString ca = u.value(QStringLiteral("created_at")).toString();
                row[QStringLiteral("created_at")] = ca.isEmpty() ? 0LL
                    : QDateTime::fromString(ca, Qt::ISODateWithMs).toMSecsSinceEpoch();
                m_users.append(row);
#ifdef QT_DEBUG
                qDebug() << "[AdminController::loadUsers] Supabase user:" << row;
#endif
            }
        } else {
#ifdef QT_DEBUG
            qWarning() << "[AdminController::loadUsers] Supabase list-users failed, status=" << status;
#endif
        }

        emit usersChanged();
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[AdminController::loadUsers] opening connection";
#endif
    QSqlDatabase db = openAdminConnection();
    if (!db.open()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::loadUsers] OPEN FAILED:" << db.lastError().text();
#endif
        closeAdminConnection(db);
        return;
    }

    QSqlQuery q(db);
    m_users.clear();

    const QString sql = QStringLiteral(
        "SELECT username, created_at FROM auth_users ORDER BY username");
#ifdef QT_DEBUG
    qDebug() << "[AdminController::loadUsers] executing:" << sql;
#endif

    if (q.exec(sql)) {
        int count = 0;
        while (q.next()) {
            QVariantMap row;
            row[QStringLiteral("username")]   = q.value(0).toString();
            row[QStringLiteral("created_at")] = q.value(1).toLongLong();
            m_users.append(row);
#ifdef QT_DEBUG
            qDebug() << "[AdminController::loadUsers] row:" << row;
#endif
            ++count;
        }
#ifdef QT_DEBUG
        qDebug() << "[AdminController::loadUsers] total rows returned:" << count;
#endif
    } else {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::loadUsers] SELECT FAILED:"
                   << q.lastError().driverText()
                   << q.lastError().databaseText();
#endif
    }

    closeAdminConnection(db);

    QSettings s(QStringLiteral("SqliteSyncPro"), QStringLiteral("SQLSyncAdmin"));
    QStringList names;
    for (const auto &v : std::as_const(m_users))
        names << v.toMap().value(QStringLiteral("username")).toString();
    s.setValue(QStringLiteral("users/list"), names);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::loadUsers] cached usernames in QSettings:" << names;
#endif

    emit usersChanged();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::loadUsers] emitted usersChanged, m_users.size()=" << m_users.size();
#endif
}

void AdminController::addUser(const QString &username, const QString &password)
{
    if (isSupabaseMode()) {
#ifdef QT_DEBUG
        qDebug() << "[AdminController::addUser] Supabase mode — calling Supabase Auth";
#endif

        if (password.length() < 8) {
#ifdef QT_DEBUG
            qWarning() << "[AdminController::addUser] password too short";
#endif
            emit userAdded(false, QStringLiteral("Password must be at least 8 characters."));
            return;
        }

        // POST /auth/v1/admin/users — email_confirm skips the email verification step
        QJsonObject body;
        body[QStringLiteral("email")]         = username;
        body[QStringLiteral("password")]      = password;
        body[QStringLiteral("email_confirm")] = true;

        int status = 0;
        const QJsonDocument resp = callSupabase(
            QStringLiteral("POST"),
            QStringLiteral("/auth/v1/admin/users"),
            QJsonDocument(body),
            &status);

        if (status >= 200 && status < 300) {
#ifdef QT_DEBUG
            qDebug() << "[AdminController::addUser] Supabase create-user success";
#endif
            emit userAdded(true, QString());
            loadUsers();
        } else {
            QString errMsg = QStringLiteral("Supabase create-user failed (HTTP %1)").arg(status);
            if (resp.isObject()) {
                const QString detail = resp.object().value(QStringLiteral("msg")).toString();
                if (!detail.isEmpty())
                    errMsg = detail;
            }
#ifdef QT_DEBUG
            qWarning() << "[AdminController::addUser]" << errMsg;
#endif
            emit userAdded(false, errMsg);
        }
        return;
    }

    const QString uname = username.toLower().trimmed();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] called with username=" << uname
             << "passwordLen=" << password.length();
#endif

    if (!isValidUsername(uname)) {
        const QString msg =
            QStringLiteral("Invalid username. Use lowercase letters, digits, and underscores only. "
                           "Must start with a letter or underscore.");
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] invalid username ->" << msg;
#endif
        emit userAdded(false, msg);
        return;
    }
    if (password.length() < 8) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] password too short";
#endif
        emit userAdded(false, QStringLiteral("Password must be at least 8 characters."));
        return;
    }

    QSqlDatabase db = openAdminConnection();
    if (!db.open()) {
        const QString err = db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] DB OPEN FAILED:" << err;
#endif
        closeAdminConnection(db);
        emit userAdded(false, err);
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] DB opened OK";
#endif

    // Use an explicit transaction so CREATE ROLE + INSERT are atomic.
    // On any failure db.rollback() undoes everything — no manual DROP ROLE needed.
    if (!db.transaction()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] BEGIN FAILED:" << db.lastError().text();
#endif
        closeAdminConnection(db);
        emit userAdded(false, QStringLiteral("Cannot start transaction: %1")
                                  .arg(db.lastError().text()));
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Transaction started (BEGIN sent)";
#endif

    QSqlQuery q(db);

    // --- Step 1: CREATE ROLE with LOGIN so the user can connect to PostgreSQL directly.
    // Single-quote any single-quotes in the password for safe interpolation into DDL.
    const QString escapedPass = QString(password).replace(QStringLiteral("'"), QStringLiteral("''"));
    const QString sqlCreate = QStringLiteral("CREATE ROLE \"%1\" LOGIN PASSWORD '%2'")
                                  .arg(uname, escapedPass);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Step 1: CREATE ROLE ... LOGIN PASSWORD '***'";
#endif
    if (!q.exec(sqlCreate)) {
        logSql("addUser/CREATE ROLE", QStringLiteral("CREATE ROLE \"%1\" LOGIN PASSWORD '***'").arg(uname), q, false);
        const QString err = q.lastError().databaseText();
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, err);
        return;
    }
    logSql("addUser/CREATE ROLE", QStringLiteral("CREATE ROLE \"%1\" LOGIN PASSWORD '***'").arg(uname), q, true);

    // --- Step 2: GRANT app_user ---
    const QString sqlGrant1 = QStringLiteral("GRANT app_user TO \"%1\"").arg(uname);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Step 2:" << sqlGrant1;
#endif
    if (!q.exec(sqlGrant1)) {
        logSql("addUser/GRANT app_user", sqlGrant1, q, false);
        const QString err = q.lastError().databaseText();
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, err);
        return;
    }
    logSql("addUser/GRANT app_user", sqlGrant1, q, true);

    // --- Step 3: GRANT user TO authenticator ---
    const QString sqlGrant2 = QStringLiteral("GRANT \"%1\" TO authenticator").arg(uname);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Step 3:" << sqlGrant2;
#endif
    if (!q.exec(sqlGrant2)) {
        logSql("addUser/GRANT TO authenticator", sqlGrant2, q, false);
        const QString err = q.lastError().databaseText();
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, err);
        return;
    }
    logSql("addUser/GRANT TO authenticator", sqlGrant2, q, true);

    // --- Step 4: Compute bcrypt hash via prepared statement ---
    // We keep prepare()+bindValue() here so the password is never interpolated into SQL.
    const QString sqlHash = QStringLiteral("SELECT crypt(:pw, gen_salt('bf', 12))");
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Step 4: compute bcrypt hash";
#endif
    QSqlQuery hashQ(db);
    if (!hashQ.prepare(sqlHash)) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] hash prepare FAILED:"
                   << hashQ.lastError().driverText()
                   << hashQ.lastError().databaseText();
#endif
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, QStringLiteral("Failed to prepare hash query: %1 — is pgcrypto installed?")
                                  .arg(hashQ.lastError().databaseText()));
        return;
    }
    hashQ.bindValue(QStringLiteral(":pw"), password);
    if (!hashQ.exec() || !hashQ.next()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] hash exec FAILED:"
                   << hashQ.lastError().driverText()
                   << hashQ.lastError().databaseText();
#endif
        const QString err = hashQ.lastError().databaseText();
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false,
            err.isEmpty() ? QStringLiteral("Failed to hash password — is pgcrypto installed?")
                          : err);
        return;
    }
    const QString hash = hashQ.value(0).toString();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] bcrypt hash length:" << hash.length()
             << "prefix:" << hash.left(10);
#endif
    if (hash.isEmpty()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] hash is empty!";
#endif
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, QStringLiteral("Password hash is empty — is pgcrypto installed?"));
        return;
    }

    // --- Step 5: INSERT into auth_users ---
    // Use exec() with inline values rather than a prepared statement to avoid
    // QPSQL server-side type-inference issues on this INSERT.
    // Safety: uname matches ^[a-z_][a-z0-9_]{0,62}$ (no SQL metacharacters);
    //         hash is bcrypt format [./A-Za-z0-9$] (no single quotes).
    const QString sqlInsert = QStringLiteral(
        "INSERT INTO auth_users (username, password_hash) VALUES ('%1', '%2')")
        .arg(uname, hash);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Step 5: INSERT username=" << uname;
#endif
    if (!q.exec(sqlInsert)) {
        logSql("addUser/INSERT auth_users", QStringLiteral("INSERT INTO auth_users (username, password_hash) VALUES ('%1', '...')").arg(uname), q, false);
        const QString err = q.lastError().databaseText();
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, err.isEmpty() ? QStringLiteral("INSERT failed.") : err);
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] INSERT rows affected:" << q.numRowsAffected();
#endif

    // --- Step 6: COMMIT ---
    if (!db.commit()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::addUser] COMMIT FAILED:" << db.lastError().text();
#endif
        db.rollback();
        closeAdminConnection(db);
        emit userAdded(false, QStringLiteral("Commit failed: %1").arg(db.lastError().text()));
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] Transaction committed successfully";
#endif

    closeAdminConnection(db);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::addUser] SUCCESS — emitting userAdded(true)";
#endif
    emit userAdded(true, QString());
    loadUsers();
}

void AdminController::removeUser(const QString &username)
{
    if (isSupabaseMode()) {
#ifdef QT_DEBUG
        qDebug() << "[AdminController::removeUser] Supabase mode, email=" << username;
#endif

        // Find the user's UUID by listing users and matching email
        int listStatus = 0;
        const QJsonDocument listResp = callSupabase(
            QStringLiteral("GET"),
            QStringLiteral("/auth/v1/admin/users"),
            QJsonDocument(),
            &listStatus);

        if (listStatus < 200 || listStatus >= 300) {
            const QString errMsg = QStringLiteral("Supabase list-users failed (HTTP %1)").arg(listStatus);
#ifdef QT_DEBUG
            qWarning() << "[AdminController::removeUser]" << errMsg;
#endif
            emit userRemoved(false, errMsg);
            return;
        }

        QString userId;
        if (listResp.isObject()) {
            const QJsonArray users = listResp.object().value(QStringLiteral("users")).toArray();
            for (const QJsonValue &v : users) {
                const QJsonObject u = v.toObject();
                if (u.value(QStringLiteral("email")).toString() == username) {
                    userId = u.value(QStringLiteral("id")).toString();
                    break;
                }
            }
        }

        if (userId.isEmpty()) {
            const QString errMsg = QStringLiteral("User '%1' not found in Supabase.").arg(username);
#ifdef QT_DEBUG
            qWarning() << "[AdminController::removeUser]" << errMsg;
#endif
            emit userRemoved(false, errMsg);
            return;
        }

        // DELETE /auth/v1/admin/users/{uuid}
        int removeStatus = 0;
        const QJsonDocument removeResp = callSupabase(
            QStringLiteral("DELETE"),
            QStringLiteral("/auth/v1/admin/users/") + userId,
            QJsonDocument(),
            &removeStatus);

        if (removeStatus >= 200 && removeStatus < 300) {
#ifdef QT_DEBUG
            qDebug() << "[AdminController::removeUser] Supabase delete success";
#endif
            emit userRemoved(true, username);
            loadUsers();
        } else {
            QString errMsg = QStringLiteral("Supabase delete-user failed (HTTP %1)").arg(removeStatus);
            if (removeResp.isObject()) {
                const QString detail = removeResp.object().value(QStringLiteral("msg")).toString();
                if (!detail.isEmpty())
                    errMsg = detail;
            }
#ifdef QT_DEBUG
            qWarning() << "[AdminController::removeUser]" << errMsg;
#endif
            emit userRemoved(false, errMsg);
        }
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[AdminController::removeUser] username=" << username;
#endif
    QSqlDatabase db = openAdminConnection();
    if (!db.open()) {
        const QString err = db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[AdminController::removeUser] OPEN FAILED:" << err;
#endif
        emit userRemoved(false, err);
        closeAdminConnection(db);
        return;
    }

    QSqlQuery q(db);

    const QString sqlDelSync =
        QStringLiteral("DELETE FROM sync_data WHERE userid = :uname");
#ifdef QT_DEBUG
    qDebug() << "[AdminController::removeUser] deleting sync_data for:" << username;
#endif
    q.prepare(sqlDelSync);
    q.bindValue(QStringLiteral(":uname"), username);
    if (q.exec()) {
        logSql("removeUser/DELETE sync_data", sqlDelSync, q, true);
    } else {
        logSql("removeUser/DELETE sync_data", sqlDelSync, q, false);
        // non-fatal — continue with auth_users removal
    }

    const QString sqlDelUser =
        QStringLiteral("DELETE FROM auth_users WHERE username = :uname");
#ifdef QT_DEBUG
    qDebug() << "[AdminController::removeUser] deleting auth_users entry";
#endif
    q.prepare(sqlDelUser);
    q.bindValue(QStringLiteral(":uname"), username);
    if (!q.exec()) {
        logSql("removeUser/DELETE auth_users", sqlDelUser, q, false);
        const QString err = q.lastError().databaseText();
        closeAdminConnection(db);
        emit userRemoved(false, err);
        return;
    }
    logSql("removeUser/DELETE auth_users", sqlDelUser, q, true);

    const QString sqlDropRole =
        QStringLiteral("DROP ROLE IF EXISTS \"%1\"").arg(username);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::removeUser] dropping role:" << sqlDropRole;
#endif
    if (!q.exec(sqlDropRole)) {
        logSql("removeUser/DROP ROLE", sqlDropRole, q, false);
        // non-fatal — role may not exist
    } else {
        logSql("removeUser/DROP ROLE", sqlDropRole, q, true);
    }

    closeAdminConnection(db);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::removeUser] SUCCESS — emitting userRemoved(true)";
#endif
    emit userRemoved(true, username);
    loadUsers();
}

void AdminController::editUser(const QString &username, const QString &newPassword)
{
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] username=" << username
             << "passwordLen=" << newPassword.length();
#endif

    if (newPassword.length() < 8) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] password too short";
#endif
        emit userEdited(false, QStringLiteral("Password must be at least 8 characters."));
        return;
    }

    if (isSupabaseMode()) {
#ifdef QT_DEBUG
        qDebug() << "[AdminController::editUser] Supabase mode, email=" << username;
#endif

        // Find the user's UUID
        int listStatus = 0;
        const QJsonDocument listResp = callSupabase(
            QStringLiteral("GET"),
            QStringLiteral("/auth/v1/admin/users"),
            QJsonDocument(),
            &listStatus);

        if (listStatus < 200 || listStatus >= 300) {
            const QString errMsg = QStringLiteral("Supabase list-users failed (HTTP %1)").arg(listStatus);
#ifdef QT_DEBUG
            qWarning() << "[AdminController::editUser]" << errMsg;
#endif
            emit userEdited(false, errMsg);
            return;
        }

        QString userId;
        if (listResp.isObject()) {
            const QJsonArray users = listResp.object().value(QStringLiteral("users")).toArray();
            for (const QJsonValue &v : users) {
                const QJsonObject u = v.toObject();
                if (u.value(QStringLiteral("email")).toString() == username) {
                    userId = u.value(QStringLiteral("id")).toString();
                    break;
                }
            }
        }

        if (userId.isEmpty()) {
            const QString errMsg = QStringLiteral("User '%1' not found in Supabase.").arg(username);
#ifdef QT_DEBUG
            qWarning() << "[AdminController::editUser]" << errMsg;
#endif
            emit userEdited(false, errMsg);
            return;
        }

        // PUT /auth/v1/admin/users/{uuid} with {password}
        QJsonObject body;
        body[QStringLiteral("password")] = newPassword;

        int updateStatus = 0;
        const QJsonDocument updateResp = callSupabase(
            QStringLiteral("PUT"),
            QStringLiteral("/auth/v1/admin/users/") + userId,
            QJsonDocument(body),
            &updateStatus);

        if (updateStatus >= 200 && updateStatus < 300) {
#ifdef QT_DEBUG
            qDebug() << "[AdminController::editUser] Supabase password update success";
#endif
            emit userEdited(true, QString());
        } else {
            QString errMsg = QStringLiteral("Supabase password update failed (HTTP %1)").arg(updateStatus);
            if (updateResp.isObject()) {
                const QString detail = updateResp.object().value(QStringLiteral("msg")).toString();
                if (!detail.isEmpty())
                    errMsg = detail;
            }
#ifdef QT_DEBUG
            qWarning() << "[AdminController::editUser]" << errMsg;
#endif
            emit userEdited(false, errMsg);
        }
        return;
    }

    QSqlDatabase db = openAdminConnection();
    if (!db.open()) {
        const QString err = db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] OPEN FAILED:" << err;
#endif
        emit userEdited(false, err);
        closeAdminConnection(db);
        return;
    }

    // Compute new hash
    const QString sqlHash = QStringLiteral("SELECT crypt(:pw, gen_salt('bf', 12))");
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] computing new hash";
#endif
    QSqlQuery hashQ(db);
    if (!hashQ.prepare(sqlHash)) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] hash prepare FAILED:"
                   << hashQ.lastError().databaseText();
#endif
        closeAdminConnection(db);
        emit userEdited(false, QStringLiteral("Failed to prepare hash: %1")
                                   .arg(hashQ.lastError().databaseText()));
        return;
    }
    hashQ.bindValue(QStringLiteral(":pw"), newPassword);
    if (!hashQ.exec() || !hashQ.next()) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] hash exec FAILED:"
                   << hashQ.lastError().databaseText();
#endif
        closeAdminConnection(db);
        const QString err = hashQ.lastError().databaseText();
        emit userEdited(false,
            err.isEmpty() ? QStringLiteral("Failed to hash password — is pgcrypto installed?")
                          : err);
        return;
    }
    const QString hash = hashQ.value(0).toString();
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] hash length:" << hash.length();
#endif

    // username was selected from the list so it's already validated;
    // hash is bcrypt format — both safe to inline.
    const QString sqlUpdate = QStringLiteral(
        "UPDATE auth_users SET password_hash = '%1' WHERE username = '%2'")
        .arg(hash, username);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] executing UPDATE for:" << username;
#endif
    QSqlQuery q(db);
    if (!q.exec(sqlUpdate)) {
        logSql("editUser/UPDATE", QStringLiteral("UPDATE auth_users SET password_hash='...' WHERE username='%1'").arg(username), q, false);
        closeAdminConnection(db);
        emit userEdited(false, q.lastError().databaseText());
        return;
    }
    logSql("editUser/UPDATE", QStringLiteral("UPDATE auth_users ... WHERE username='%1'").arg(username), q, true);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] rows affected:" << q.numRowsAffected();
#endif

    if (q.numRowsAffected() == 0) {
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] zero rows updated — user not found?";
#endif
        closeAdminConnection(db);
        emit userEdited(false, QStringLiteral("User not found."));
        return;
    }

    // Also update the PostgreSQL role password so the user can still connect directly.
    const QString escapedPass = QString(newPassword).replace(QStringLiteral("'"), QStringLiteral("''"));
    const QString sqlAlter = QStringLiteral("ALTER ROLE \"%1\" PASSWORD '%2'")
                                 .arg(username, escapedPass);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] ALTER ROLE ... PASSWORD '***'";
#endif
    if (!q.exec(sqlAlter)) {
        logSql("editUser/ALTER ROLE", QStringLiteral("ALTER ROLE \"%1\" PASSWORD '***'").arg(username), q, false);
        // Non-fatal: auth_users was updated; log but don't fail the whole operation.
#ifdef QT_DEBUG
        qWarning() << "[AdminController::editUser] ALTER ROLE failed (non-fatal):"
                   << q.lastError().databaseText();
#endif
    } else {
        logSql("editUser/ALTER ROLE", QStringLiteral("ALTER ROLE \"%1\" PASSWORD '***'").arg(username), q, true);
    }

    closeAdminConnection(db);
#ifdef QT_DEBUG
    qDebug() << "[AdminController::editUser] SUCCESS";
#endif
    emit userEdited(true, QString());
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – clipboard
// ---------------------------------------------------------------------------

void AdminController::copyToClipboard(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE – postgrest config snippet
// ---------------------------------------------------------------------------

QString AdminController::postgrestConfig() const
{
    if (isSupabaseMode()) {
        // In Supabase mode: PostgREST uses app_user role for all requests;
        // Supabase Auth issues JWTs verified by PostgREST using the Supabase JWT secret.
        // The jwt-secret must match the JWT secret shown in your Supabase project settings
        // (Settings → API → JWT Settings → JWT Secret).
        return QStringLiteral(
                   "db-uri = \"postgres://%1:%2@%3:%4/%5\"\n"
                   "db-schema = \"public\"\n"
                   "db-anon-role = \"anon\"\n"
                   "# Set jwt-secret to your Supabase JWT secret (Settings → API → JWT Settings):\n"
                   "jwt-secret = \"<your-supabase-jwt-secret>\"\n"
                   "server-host = \"*\"\n"
                   "server-port = 3000\n")
            .arg(m_superuser)
            .arg(m_superPass)
            .arg(m_host)
            .arg(m_port)
            .arg(m_dbName);
    }

    return QStringLiteral(
               "db-uri = \"postgres://authenticator:%1@%2:%3/%4\"\n"
               "db-schema = \"public\"\n"
               "db-anon-role = \"anon\"\n"
               "jwt-secret = \"%5\"\n"
               "server-host = \"*\"\n"
               "server-port = 3000\n")
        .arg(m_authenticatorPassword)
        .arg(m_host)
        .arg(m_port)
        .arg(m_dbName)
        .arg(m_jwtSecret);
}
