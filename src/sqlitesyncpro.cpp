// Copyright (C) 2026 Paul McKinney
#include "sqlitesyncpro.h"
#include "authmanager.h"
#include "httpclient.h"
#include "syncengine.h"
#include "syncloopworker.h"
#include "syncworker.h"
#include "schemainspector.h"
#include "syncapisettingsdialog.h"
#include "syncstatswindow.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QReadWriteLock>
#include <QThread>
#include <QUuid>
#include <QMutexLocker>
#include <QCryptographicHash>
#include <QDebug>

// ---------------------------------------------------------------------------
// Extract the "sub" claim from a JWT without verifying the signature.
// ---------------------------------------------------------------------------
static QString jwtSubject(const QString &token)
{
    const QStringList parts = token.split(QLatin1Char('.'));
    if (parts.size() < 2)
        return {};

    const QByteArray payload = QByteArray::fromBase64(
        parts.at(1).toLatin1(),
        QByteArray::Base64UrlEncoding);

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return {};

    return doc.object().value(QStringLiteral("sub")).toString();
}

// ---------------------------------------------------------------------------
// SqliteSyncPro
// ---------------------------------------------------------------------------

SqliteSyncPro::SqliteSyncPro(QObject *parent)
    : QObject(parent)
{
}

SqliteSyncPro::~SqliteSyncPro()
{
    if (m_syncThread) {
        // Disconnect our callbacks so queued signals don't fire into a dead object.
        if (m_syncWorker)
            QObject::disconnect(m_syncWorker, nullptr, this, nullptr);
        QObject::disconnect(m_syncThread, nullptr, this, nullptr);

        if (m_syncWorker)
            m_syncWorker->requestStop();
        m_syncThread->quit();
        m_syncThread->wait();   // blocks until the thread exits cleanly

        delete m_syncWorker;
        m_syncWorker = nullptr;
        delete m_syncThread;
        m_syncThread = nullptr;
    }

    if (m_persistentDb.isOpen()) {
        m_persistentDb.close();
        m_persistentDb = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_persistentConnName);
    }

    if (m_statsWindow) {
        delete m_statsWindow;
        m_statsWindow = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Settings – setters
// ---------------------------------------------------------------------------

void SqliteSyncPro::setSyncHostType(int type)
{
    QMutexLocker lock(&m_mutex);
    m_syncHostType = type;
}

void SqliteSyncPro::setPostgrestUrl(const QString &url)
{
    QMutexLocker lock(&m_mutex);
    m_postgrestUrl = url;
}

void SqliteSyncPro::setEmail(const QString &email)
{
    QMutexLocker lock(&m_mutex);
    m_email = email;
}

void SqliteSyncPro::setPassword(const QString &password)
{
    QMutexLocker lock(&m_mutex);
    m_password = password;
}

void SqliteSyncPro::setSupabaseKey(const QString &key)
{
    QMutexLocker lock(&m_mutex);
    m_supabaseKey = key;
}

void SqliteSyncPro::setEncryptionPhrase(const QString &phrase)
{
    QMutexLocker lock(&m_mutex);
    m_encryptionPhrase = phrase;
    if (phrase.isEmpty()) {
        m_encryptionKey.clear();
    } else {
        m_encryptionKey =
            QCryptographicHash::hash(phrase.toUtf8(), QCryptographicHash::Sha256);
    }
}

void SqliteSyncPro::setDatabasePath(const QString &path)
{
    QMutexLocker lock(&m_mutex);
    m_databasePath = path;
}

void SqliteSyncPro::setPostgresTableName(const QString &tableName)
{
    QMutexLocker lock(&m_mutex);
    m_postgresTableName = tableName.isEmpty()
                              ? QStringLiteral("sync_data")
                              : tableName;
}

void SqliteSyncPro::setSyncIntervalMs(int ms)
{
    QMutexLocker lock(&m_mutex);
    m_syncIntervalMs = ms;
}

// ---------------------------------------------------------------------------
// Settings – getters
// ---------------------------------------------------------------------------

int SqliteSyncPro::syncHostType() const
{
    QMutexLocker lock(&m_mutex);
    return m_syncHostType;
}

QString SqliteSyncPro::postgrestUrl() const
{
    QMutexLocker lock(&m_mutex);
    return m_postgrestUrl;
}

QString SqliteSyncPro::email() const
{
    QMutexLocker lock(&m_mutex);
    return m_email;
}

QString SqliteSyncPro::password() const
{
    QMutexLocker lock(&m_mutex);
    return m_password;
}

QString SqliteSyncPro::supabaseKey() const
{
    QMutexLocker lock(&m_mutex);
    return m_supabaseKey;
}

QString SqliteSyncPro::encryptionPhrase() const
{
    QMutexLocker lock(&m_mutex);
    return m_encryptionPhrase;
}

QString SqliteSyncPro::databasePath() const
{
    QMutexLocker lock(&m_mutex);
    return m_databasePath;
}

QString SqliteSyncPro::postgresTableName() const
{
    QMutexLocker lock(&m_mutex);
    return m_postgresTableName;
}

int SqliteSyncPro::syncIntervalMs() const
{
    QMutexLocker lock(&m_mutex);
    return m_syncIntervalMs;
}

// ---------------------------------------------------------------------------
// Settings dialog
// ---------------------------------------------------------------------------

bool SqliteSyncPro::showSettingsDialog(QWidget *parent)
{
    SyncAPISettingsDialog dlg(this, parent);
    return dlg.exec() == QDialog::Accepted;
}

// ---------------------------------------------------------------------------
// Stats window
// ---------------------------------------------------------------------------

void SqliteSyncPro::showStats(bool show)
{
    if (show) {
        if (!m_statsWindow) {
            m_statsWindow = new SyncStatsWindow;
            connect(this,          &SqliteSyncPro::syncCompleted,
                    m_statsWindow, &SyncStatsWindow::addDataPoint);
            connect(m_statsWindow, &SyncStatsWindow::windowClosed,
                    this,          &SqliteSyncPro::statsWindowClosed);
        }
        m_statsWindow->show();
        m_statsWindow->raise();
        m_statsWindow->activateWindow();
    } else {
        if (m_statsWindow)
            m_statsWindow->hide();
    }
}

bool SqliteSyncPro::isStatsVisible() const
{
    return m_statsWindow && m_statsWindow->isVisible();
}

// ---------------------------------------------------------------------------
// Encryption key (raw bytes, alternative to setEncryptionPhrase)
// ---------------------------------------------------------------------------

void SqliteSyncPro::setEncryptionKey(const QByteArray &key32)
{
    QMutexLocker lock(&m_mutex);
    m_encryptionKey = key32;
}

void SqliteSyncPro::clearEncryptionKey()
{
    QMutexLocker lock(&m_mutex);
    m_encryptionKey.clear();
    m_encryptionPhrase.clear();
}

bool SqliteSyncPro::hasEncryptionKey() const
{
    QMutexLocker lock(&m_mutex);
    return !m_encryptionKey.isEmpty();
}

// ---------------------------------------------------------------------------
// Table configuration
// ---------------------------------------------------------------------------

void SqliteSyncPro::addTable(const QString &tableName, int batchSize)
{
    SyncTableConfig cfg;
    cfg.tableName = tableName;
    cfg.batchSize = batchSize;
    QMutexLocker lock(&m_mutex);
    m_tables.append(cfg);
}

void SqliteSyncPro::addTable(const SyncTableConfig &config)
{
    QMutexLocker lock(&m_mutex);
    m_tables.append(config);
}

void SqliteSyncPro::clearTables()
{
    QMutexLocker lock(&m_mutex);
    m_tables.clear();
}

// ---------------------------------------------------------------------------
// Private: authenticate using current credential settings
// ---------------------------------------------------------------------------

bool SqliteSyncPro::doAuthenticate()
{
    // Snapshot settings under the lock.
    int     hostType;
    QString url, email, password, supabaseKey, encPhrase;
    {
        QMutexLocker lock(&m_mutex);
        hostType    = m_syncHostType;
        url         = m_postgrestUrl;
        email       = m_email;
        password    = m_password;
        supabaseKey = m_supabaseKey;
        encPhrase   = m_encryptionPhrase;
    }

    if (url.isEmpty() || email.isEmpty()) {
        QMutexLocker lock(&m_mutex);
        m_lastError = QStringLiteral(
            "Sync settings incomplete (url='%1' email='%2'). "
            "Configure via setters or showSettingsDialog().")
            .arg(url, email);
        return false;
    }

    QString authToken, userId, resolvedSupabaseKey;

    if (hostType == 1) {
        // Supabase
        const QString postgrestUrl = url + QStringLiteral("/rest/v1");
        HttpClient http;
        http.setBaseUrl(postgrestUrl);
        http.setApiKey(supabaseKey);

        const QString authUrl = url.trimmed()
                              + QStringLiteral("/auth/v1/token?grant_type=password");

        QString authError;
        AuthManager auth;
        QObject::connect(&auth, &AuthManager::authenticationFailed,
                         &auth, [&authError](const QString &reason) { authError = reason; });
        if (!auth.login(&http, authUrl, email, password)) {
            QMutexLocker lock(&m_mutex);
            m_lastError = authError.isEmpty()
                              ? QStringLiteral("Supabase authentication failed") : authError;
            return false;
        }
        authToken          = auth.token();
        userId             = jwtSubject(authToken);
        resolvedSupabaseKey = supabaseKey;

        QMutexLocker lock(&m_mutex);
        m_postgrestUrl = postgrestUrl;  // store the resolved PostgREST URL
        m_authToken    = authToken;
        m_supabaseKey  = resolvedSupabaseKey;
        if (!userId.isEmpty()) m_userId = userId;
        m_lastError.clear();
    } else {
        // Self-hosted
        HttpClient http;
        http.setBaseUrl(url);

        QString authError;
        AuthManager auth;
        QObject::connect(&auth, &AuthManager::authenticationFailed,
                         &auth, [&authError](const QString &reason) { authError = reason; });
        if (!auth.login(&http, QStringLiteral("rpc/rpc_login"), email, password)) {
            QMutexLocker lock(&m_mutex);
            m_lastError = authError.isEmpty()
                              ? QStringLiteral("Authentication failed") : authError;
            return false;
        }
        authToken = auth.token();
        userId    = jwtSubject(authToken);

        QMutexLocker lock(&m_mutex);
        m_authToken = authToken;
        if (!userId.isEmpty()) m_userId = userId;
        m_lastError.clear();
    }

    return true;
}

// ---------------------------------------------------------------------------
// Private: discover tables that have the required sync columns
// ---------------------------------------------------------------------------

QStringList SqliteSyncPro::discoverSyncTables(const QString &dbPath)
{
    const QString connName = QStringLiteral("SqliteSyncPro_discover_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    auto cleanup = [&db, &connName]() {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    };

    if (!db.open()) {
        cleanup();
        return {};
    }

    // Get all user tables (exclude SQLite internal tables and our _sync_meta).
    // Scope q so it is destroyed before cleanup() calls removeDatabase().
    QStringList allTables;
    {
        QSqlQuery q(db);
        q.exec(QStringLiteral(
            "SELECT name FROM sqlite_master "
            "WHERE type='table' "
            "  AND name NOT LIKE 'sqlite_%' "
            "  AND name != '_sync_meta' "
            "ORDER BY name"));

        while (q.next())
            allTables << q.value(0).toString();
    } // q destroyed here

    cleanup();

    // Filter to tables that have id, updateddate, syncdate.
    const QString checkConn = QStringLiteral("SqliteSyncPro_check_%1")
                                  .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase checkDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), checkConn);
    checkDb.setDatabaseName(dbPath);

    auto cleanup2 = [&checkDb, &checkConn]() {
        checkDb.close();
        checkDb = QSqlDatabase();
        QSqlDatabase::removeDatabase(checkConn);
    };

    if (!checkDb.open()) {
        cleanup2();
        return {};
    }

    QStringList syncTables;

    {
        // Scope inspector so it releases its QSqlDatabase copy before cleanup2() calls
        // removeDatabase() — otherwise Qt warns about the connection still being in use.
        SchemaInspector inspector(checkDb);

        for (const QString &tableName : allTables) {
            QString errMsg;
            if (inspector.hasRequiredColumns(tableName,
                                             QStringLiteral("id"),
                                             QStringLiteral("updateddate"),
                                             QStringLiteral("syncdate"),
                                             errMsg)) {
                syncTables << tableName;
#ifdef QT_DEBUG
                qDebug() << "[SqliteSyncPro] Will sync table:" << tableName;
#endif
            } else {
#ifdef QT_DEBUG
                qDebug() << "[SqliteSyncPro] Skipping table" << tableName << "-" << errMsg;
#endif
            }
        }
    } // inspector destroyed here; checkDb is the only remaining reference

    cleanup2();
    return syncTables;
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

bool SqliteSyncPro::initialize()
{
    // If already initialized, shut down first.
    if (m_syncThread || m_persistentDb.isOpen())
        shutdown();

    // Snapshot the database path.
    QString dbPath;
    {
        QMutexLocker lock(&m_mutex);
        dbPath = m_databasePath;
    }

    if (dbPath.isEmpty()) {
        QMutexLocker lock(&m_mutex);
        m_lastError = QStringLiteral("Database path not set. Call setDatabasePath() first.");
        return false;
    }

    // Authenticate.
    if (!doAuthenticate())
        return false;

    // Open a temporary connection to set WAL mode.
    {
        const QString walConn = QStringLiteral("SqliteSyncPro_wal_%1")
                                    .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QSqlDatabase walDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), walConn);
        walDb.setDatabaseName(dbPath);
        if (walDb.open()) {
            QSqlQuery wq(walDb);
            wq.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
            walDb.close();
        }
        walDb = QSqlDatabase();
        QSqlDatabase::removeDatabase(walConn);
    }

    // Discover tables.
    const QStringList tableNames = discoverSyncTables(dbPath);

    {
        QMutexLocker lock(&m_mutex);
        m_tables.clear();
        for (const QString &name : tableNames) {
            SyncTableConfig cfg;
            cfg.tableName = name;
            m_tables.append(cfg);
        }
        m_dbOpen = true;
        m_lastError.clear();
    }

    // Open the persistent connection for the calling application.
    m_persistentConnName = QStringLiteral("SqliteSyncPro_main_%1")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_persistentDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_persistentConnName);
    m_persistentDb.setDatabaseName(dbPath);
    if (!m_persistentDb.open()) {
        const QString err = QStringLiteral("Cannot open persistent database connection: %1")
                                .arg(m_persistentDb.lastError().text());
        m_persistentDb = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_persistentConnName);
        QMutexLocker lock(&m_mutex);
        m_lastError = err;
        m_dbOpen    = false;
        return false;
    }

    // Snapshot config for the loop worker.
    QString serverUrl, authToken, supabaseKey, userId, postgresTableName;
    QString email, password;
    QByteArray encryptionKey;
    QList<SyncTableConfig> tables;
    int syncIntervalMs, syncHostType;
    {
        QMutexLocker lock(&m_mutex);
        serverUrl         = m_postgrestUrl;
        authToken         = m_authToken;
        supabaseKey       = m_supabaseKey;
        userId            = m_userId;
        postgresTableName = m_postgresTableName;
        encryptionKey     = m_encryptionKey;
        tables            = m_tables;
        syncIntervalMs    = m_syncIntervalMs;
        syncHostType      = m_syncHostType;
        email             = m_email;
        password          = m_password;
    }

    // Start the background loop thread (no parent — destructor deletes it explicitly).
    m_syncThread = new QThread;
    m_syncWorker = new SyncLoopWorker(dbPath, serverUrl, authToken, supabaseKey,
                                      userId, postgresTableName, tables,
                                      m_dbLock, encryptionKey, syncIntervalMs,
                                      syncHostType, email, password);
    m_syncWorker->moveToThread(m_syncThread);

    connect(m_syncThread, &QThread::started,         m_syncWorker, &SyncLoopWorker::run);
    connect(m_syncWorker, &SyncLoopWorker::progress,
            this,          &SqliteSyncPro::syncProgress);
    connect(m_syncWorker, &SyncLoopWorker::syncCompleted,
            this,          &SqliteSyncPro::syncCompleted);
    connect(m_syncWorker, &SyncLoopWorker::rowChanged,
            this,          &SqliteSyncPro::rowChanged);
    connect(m_syncWorker, &SyncLoopWorker::authenticationRequired,
            this,          &SqliteSyncPro::authenticationRequired);

    connect(m_syncWorker, &SyncLoopWorker::finished, this, [this]() {
        // Worker has exited its loop; quit the thread so QThread::finished fires.
        m_syncThread->quit();
    });
    connect(m_syncThread, &QThread::finished, this, [this]() {
        // Thread has fully stopped.  Clean up and notify the calling application.
        delete m_syncWorker;
        m_syncWorker = nullptr;
        delete m_syncThread;
        m_syncThread = nullptr;
        {
            QMutexLocker lock(&m_mutex);
            m_initialized = false;
        }
        emit syncStopped();
    });

    m_syncThread->start();

    {
        QMutexLocker lock(&m_mutex);
        m_initialized = true;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Database access
// ---------------------------------------------------------------------------

QSqlDatabase SqliteSyncPro::database() const
{
    return m_persistentDb;
}

QReadWriteLock *SqliteSyncPro::databaseLock() const
{
    QMutexLocker lock(&m_mutex);
    return m_dbLock;
}

// ---------------------------------------------------------------------------
// Backwards-compatible helpers
// ---------------------------------------------------------------------------

bool SqliteSyncPro::authenticateWithToken(const QString &serverUrl, const QString &jwtToken)
{
    if (jwtToken.trimmed().isEmpty()) {
        QMutexLocker lock(&m_mutex);
        m_lastError = QStringLiteral("Empty token provided");
        return false;
    }
    QMutexLocker lock(&m_mutex);
    m_postgrestUrl = serverUrl;
    m_authToken    = jwtToken;
    m_lastError.clear();
    return true;
}

bool SqliteSyncPro::openDatabase(const QString &dbPath)
{
    // Validate the path and set WAL mode.
    const QString connName = QStringLiteral("SqliteSyncPro_init_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        const QString err = QStringLiteral("Cannot open database '%1': %2")
                                .arg(dbPath, db.lastError().text());
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        QMutexLocker lock(&m_mutex);
        m_lastError = err;
        return false;
    }

    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);

    QMutexLocker lock(&m_mutex);
    m_databasePath = dbPath;
    m_dbOpen       = true;
    m_lastError.clear();
    return true;
}

void SqliteSyncPro::setDatabaseLock(QReadWriteLock *lock)
{
    QMutexLocker locker(&m_mutex);
    m_dbLock = lock ? lock : &m_internalLock;
}

void SqliteSyncPro::synchronizeAsync()
{
    QString dbPath, serverUrl, authToken, supabaseKey, userId, postgresTableName;
    QByteArray encryptionKey;
    QList<SyncTableConfig> tables;
    QReadWriteLock *dbLock = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        dbPath            = m_databasePath;
        serverUrl         = m_postgrestUrl;
        authToken         = m_authToken;
        supabaseKey       = m_supabaseKey;
        userId            = m_userId;
        postgresTableName = m_postgresTableName;
        encryptionKey     = m_encryptionKey;
        tables            = m_tables;
        dbLock            = m_dbLock;
    }

    auto *thread = new QThread;
    auto *worker = new SyncWorker(dbPath, serverUrl, authToken, supabaseKey,
                                  userId, postgresTableName, tables,
                                  dbLock, encryptionKey);
    worker->moveToThread(thread);
    connect(thread, &QThread::started,         worker, &SyncWorker::run);
    connect(worker, &SyncWorker::finished,     this,   &SqliteSyncPro::syncCompleted);
    connect(worker, &SyncWorker::progress,     this,   &SqliteSyncPro::syncProgress);
    connect(worker, &SyncWorker::finished,     worker, &QObject::deleteLater);
    connect(worker, &SyncWorker::finished,     thread, &QThread::quit);
    connect(thread, &QThread::finished,        thread, &QObject::deleteLater);
    thread->start();
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void SqliteSyncPro::shutdown()
{
    if (m_syncWorker)
        m_syncWorker->requestStop();
    // The syncStopped signal is emitted asynchronously when the thread exits.
}

void SqliteSyncPro::retryNow()
{
    // Invoke on the worker's thread so the mutex-protected wakeAll() is safe.
    if (m_syncWorker)
        QMetaObject::invokeMethod(m_syncWorker, "retryNow", Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// Manual synchronization helpers
// ---------------------------------------------------------------------------

SyncResult SqliteSyncPro::runSync(const QList<SyncTableConfig> &tables)
{
    SyncResult combined;

    QString dbPath, serverUrl, authToken, supabaseKey, userId, postgresTableName;
    QByteArray encryptionKey;
    QReadWriteLock *dbLock = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_dbOpen) {
            combined.success      = false;
            combined.errorMessage = QStringLiteral("No database open. Call initialize() first.");
            m_lastError           = combined.errorMessage;
            return combined;
        }
        if (m_authToken.isEmpty()) {
            combined.success      = false;
            combined.errorMessage = QStringLiteral("Not authenticated.");
            m_lastError           = combined.errorMessage;
            return combined;
        }
        dbPath            = m_databasePath;
        serverUrl         = m_postgrestUrl;
        authToken         = m_authToken;
        supabaseKey       = m_supabaseKey;
        userId            = m_userId;
        postgresTableName = m_postgresTableName;
        encryptionKey     = m_encryptionKey;
        dbLock            = m_dbLock;
    }

    const QString connName = QStringLiteral("SqliteSyncPro_sync_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    auto cleanupDb = [&db, &connName]() {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    };

    if (!db.open()) {
        combined.success      = false;
        combined.errorMessage = QStringLiteral("Cannot open database for sync: %1")
                                    .arg(db.lastError().text());
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        QMutexLocker lock(&m_mutex);
        m_lastError = combined.errorMessage;
        return combined;
    }

    HttpClient http;
    http.setBaseUrl(serverUrl);
    http.setAuthToken(authToken);
    http.setApiKey(supabaseKey);

    SyncEngine engine;
    engine.setDatabase(db);
    engine.setHttpClient(&http);
    engine.setUserId(userId);
    engine.setPostgresTableName(postgresTableName);
    engine.setDatabaseLock(dbLock);
    engine.setEncryptionKey(encryptionKey);

    connect(&engine, &SyncEngine::progress,   this, &SqliteSyncPro::syncProgress);
    connect(&engine, &SyncEngine::rowChanged, this, &SqliteSyncPro::rowChanged);

    for (const auto &cfg : tables) {
        const SyncResult tableSync = engine.synchronizeTable(cfg);
        for (const auto &tr : tableSync.tableResults)
            combined.tableResults.append(tr);
        if (!tableSync.success) {
            combined.success      = false;
            combined.errorMessage = tableSync.errorMessage;
        }
    }

    cleanupDb();

    {
        QMutexLocker lock(&m_mutex);
        m_lastError = combined.success ? QString() : combined.errorMessage;
    }

    return combined;
}

SyncResult SqliteSyncPro::synchronize()
{
    QList<SyncTableConfig> tables;
    {
        QMutexLocker lock(&m_mutex);
        tables = m_tables;
    }
    const SyncResult result = runSync(tables);
    emit syncCompleted(result);
    return result;
}

SyncResult SqliteSyncPro::synchronizeTable(const QString &tableName)
{
    QList<SyncTableConfig> match;
    {
        QMutexLocker lock(&m_mutex);
        for (const auto &cfg : m_tables) {
            if (cfg.tableName == tableName) {
                match.append(cfg);
                break;
            }
        }
    }

    if (match.isEmpty()) {
        SyncResult result;
        result.success      = false;
        result.errorMessage = QStringLiteral("Table '%1' not found.").arg(tableName);
        QMutexLocker lock(&m_mutex);
        m_lastError = result.errorMessage;
        return result;
    }

    const SyncResult result = runSync(match);
    emit syncCompleted(result);
    return result;
}

// ---------------------------------------------------------------------------
// Sync status check (background worker)
// ---------------------------------------------------------------------------

class SyncStatusWorker : public QObject
{
    Q_OBJECT
public:
    SyncStatusWorker(const QString &dbPath,
                     const QString &serverUrl,
                     const QString &authToken,
                     const QString &supabaseKey,
                     const QString &postgresTableName,
                     const QList<SyncTableConfig> &tables,
                     QReadWriteLock *dbLock,
                     bool pullCaughtUp)
        : m_dbPath(dbPath)
        , m_serverUrl(serverUrl)
        , m_authToken(authToken)
        , m_supabaseKey(supabaseKey)
        , m_postgresTableName(postgresTableName)
        , m_tables(tables)
        , m_dbLock(dbLock)
        , m_pullCaughtUp(pullCaughtUp)
    {}

signals:
    void done(int percentComplete, qint64 pendingPush, qint64 pendingPull);

public slots:
    void run()
    {
        const QString connName = QStringLiteral("SyncStatus_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(m_dbPath);

        if (!db.open()) {
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            emit done(0, 0, 0);
            return;
        }

        qint64 totalLocal  = 0;
        qint64 pendingPush = 0;

        {
            QReadLocker lock(m_dbLock);
            for (const SyncTableConfig &cfg : m_tables) {
                QSqlQuery q(db);
                q.exec(QStringLiteral("SELECT COUNT(*) FROM \"%1\"").arg(cfg.tableName));
                if (q.next())
                    totalLocal += q.value(0).toLongLong();

                q.exec(QStringLiteral("SELECT COUNT(*) FROM \"%1\" WHERE syncdate IS NULL")
                           .arg(cfg.tableName));
                if (q.next())
                    pendingPush += q.value(0).toLongLong();
            }
        }

        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);

        // If the pull is caught up (last cycle pulled 0 with no network error) we
        // only need to consider local dirty records.  Skip the server HEAD queries
        // entirely — they can return a larger total than local for legitimate reasons
        // (other clients, conflict-skipped records) and would cause a false < 100%.
        if (m_pullCaughtUp) {
            const int percent = (totalLocal == 0 || pendingPush == 0)
                                    ? 100
                                    : static_cast<int>((totalLocal - pendingPush) * 100 / totalLocal);
            emit done(qBound(0, percent, 100), pendingPush, 0LL);
            return;
        }

        // Pull is still in progress — query server to compute how far along we are.
        HttpClient http;
        http.setBaseUrl(m_serverUrl);
        http.setAuthToken(m_authToken);
        http.setApiKey(m_supabaseKey);

        qint64 totalServer = 0;
        for (const SyncTableConfig &cfg : m_tables) {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("tablename"),
                               QStringLiteral("eq.%1").arg(cfg.tableName));
            const int count = http.countRows(m_postgresTableName, query);
            if (count >= 0)
                totalServer += count;
        }

        const qint64 syncedLocal = totalLocal - pendingPush;
        const qint64 grandTotal  = qMax(totalLocal, totalServer);
        const qint64 pendingPull = qMax(0LL, totalServer - totalLocal);
        int percent = (grandTotal == 0) ? 100
                                        : static_cast<int>(syncedLocal * 100 / grandTotal);
        emit done(qBound(0, percent, 100), pendingPush, pendingPull);
    }

private:
    QString                m_dbPath;
    QString                m_serverUrl;
    QString                m_authToken;
    QString                m_supabaseKey;
    QString                m_postgresTableName;
    QList<SyncTableConfig> m_tables;
    QReadWriteLock        *m_dbLock;
    bool                   m_pullCaughtUp;
};

void SqliteSyncPro::checkSyncStatus(const SyncResult &lastResult)
{
    QString dbPath, serverUrl, authToken, supabaseKey, postgresTableName;
    QList<SyncTableConfig> tables;
    QReadWriteLock *dbLock = nullptr;

    {
        QMutexLocker lock(&m_mutex);
        if (!m_initialized || m_tables.isEmpty()) {
            emit syncStatusUpdated(100, 0LL, 0LL);
            return;
        }
        dbPath            = m_databasePath;
        serverUrl         = m_postgrestUrl;
        authToken         = m_authToken;
        supabaseKey       = m_supabaseKey;
        postgresTableName = m_postgresTableName;
        tables            = m_tables;
        dbLock            = m_dbLock;
    }

    // Pull is "caught up" when the last cycle pulled nothing and had no network error.
    const bool pullCaughtUp = (lastResult.totalPulled() == 0 && !lastResult.hasNetworkError());

    auto *thread = new QThread;
    auto *worker = new SyncStatusWorker(dbPath, serverUrl, authToken, supabaseKey,
                                        postgresTableName, tables, dbLock, pullCaughtUp);
    worker->moveToThread(thread);

    connect(thread, &QThread::started,       worker, &SyncStatusWorker::run);
    connect(worker, &SyncStatusWorker::done, this,   &SqliteSyncPro::syncStatusUpdated,
            Qt::QueuedConnection);
    connect(worker, &SyncStatusWorker::done, worker,
            [worker](int, qint64, qint64){ worker->deleteLater(); });
    connect(worker, &SyncStatusWorker::done, thread,
            [thread](int, qint64, qint64){ thread->quit(); });
    connect(thread, &QThread::finished,       thread, &QObject::deleteLater);

    thread->start();
}

bool SqliteSyncPro::syncAll()
{
    QList<SyncTableConfig> tables;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_dbOpen || m_tables.isEmpty())
            return false;
        tables = m_tables;
    }

    QWriteLocker dbLock(m_dbLock);

    if (!m_persistentDb.isOpen())
        return false;

    m_persistentDb.transaction();

    // Mark every row in every sync table as dirty so it gets pushed
    for (const SyncTableConfig &cfg : tables) {
        QSqlQuery q(m_persistentDb);
        q.prepare(QStringLiteral("UPDATE \"%1\" SET syncdate = NULL").arg(cfg.tableName));
#ifdef QT_DEBUG
        if (!q.exec())
            qWarning().noquote()
                << QStringLiteral("[SqliteSyncPro] syncAll: failed to reset syncdate for '%1': %2")
                       .arg(cfg.tableName, q.lastError().text());
#endif
    }

    // Reset all pull high-water marks so every table re-pulls from the beginning
    {
        QSqlQuery q(m_persistentDb);
        q.exec(QStringLiteral("DELETE FROM _sync_meta"));
    }

    m_persistentDb.commit();

#ifdef QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SqliteSyncPro] syncAll: %1 table(s) marked for full resync")
                              .arg(tables.size());
#endif
    return true;
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

bool SqliteSyncPro::isAuthenticated() const
{
    QMutexLocker lock(&m_mutex);
    return !m_authToken.isEmpty();
}

bool SqliteSyncPro::isDatabaseOpen() const
{
    QMutexLocker lock(&m_mutex);
    return m_dbOpen;
}

bool SqliteSyncPro::isInitialized() const
{
    QMutexLocker lock(&m_mutex);
    return m_initialized;
}

QString SqliteSyncPro::lastError() const
{
    QMutexLocker lock(&m_mutex);
    return m_lastError;
}

#include "sqlitesyncpro.moc"
