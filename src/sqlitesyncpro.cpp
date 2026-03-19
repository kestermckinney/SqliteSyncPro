#include "sqlitesyncpro.h"
#include "authmanager.h"
#include "httpclient.h"
#include "syncengine.h"
#include "syncworker.h"
#include "syncapisettingsdialog.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QReadWriteLock>
#include <QThread>
#include <QUuid>
#include <QMutexLocker>
#include <QSettings>
#include <QSysInfo>
#include <QCryptographicHash>
#include <QDebug>

// ---------------------------------------------------------------------------
// Extract the "sub" claim from a JWT without verifying the signature.
// Used to auto-populate m_userId after a successful login.
// ---------------------------------------------------------------------------
static QString jwtSubject(const QString &token)
{
    const QStringList parts = token.split(QLatin1Char('.'));
    if (parts.size() < 2)
        return {};

    // JWT payload is base64url-encoded JSON (padding may be absent).
    const QByteArray payload = QByteArray::fromBase64(
        parts.at(1).toLatin1(),
        QByteArray::Base64UrlEncoding);

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return {};

    return doc.object().value(QStringLiteral("sub")).toString();
}

// ---------------------------------------------------------------------------
// Settings helpers — mirror the obfuscation used in SyncAPISettingsDialog.
// ---------------------------------------------------------------------------

static QByteArray settingsObfuscationKey()
{
    QByteArray uid = QSysInfo::machineUniqueId();
    if (uid.isEmpty())
        uid = QByteArrayLiteral("sqlitesyncpro-fallback-id");
    return QCryptographicHash::hash(uid, QCryptographicHash::Sha256);
}

static QString settingsDeobfuscate(const QString &stored)
{
    if (stored.isEmpty()) return {};
    QByteArray data = QByteArray::fromBase64(stored.toLatin1());
    const QByteArray key = settingsObfuscationKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromUtf8(data);
}

static constexpr char kSettingsOrg[]   = "ProjectNotes";
static constexpr char kSettingsApp[]   = "AppSettings";
static constexpr char kSettingsGroup[] = "sync_api";

// ---------------------------------------------------------------------------
// SqliteSyncPro
// ---------------------------------------------------------------------------

SqliteSyncPro::SqliteSyncPro(QObject *parent)
    : QObject(parent)
{
}

// ---------------------------------------------------------------------------
// User identity and Postgres table name
// ---------------------------------------------------------------------------

void SqliteSyncPro::setUserId(const QString &userId)
{
    QMutexLocker lock(&m_mutex);
    m_userId = userId;
}

void SqliteSyncPro::setPostgresTableName(const QString &tableName)
{
    QMutexLocker lock(&m_mutex);
    m_postgresTableName = tableName.isEmpty() ? QStringLiteral("sync_data") : tableName;
}

// ---------------------------------------------------------------------------
// Database lock
// ---------------------------------------------------------------------------

QReadWriteLock *SqliteSyncPro::databaseLock() const
{
    QMutexLocker lock(&m_mutex);
    return m_dbLock;
}

void SqliteSyncPro::setDatabaseLock(QReadWriteLock *lock)
{
    QMutexLocker locker(&m_mutex);
    m_dbLock = lock ? lock : &m_internalLock;
}

// ---------------------------------------------------------------------------
// Encryption key
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
}

bool SqliteSyncPro::hasEncryptionKey() const
{
    QMutexLocker lock(&m_mutex);
    return !m_encryptionKey.isEmpty();
}

// ---------------------------------------------------------------------------
// Settings dialog + authenticate from settings
// ---------------------------------------------------------------------------

bool SqliteSyncPro::showSettingsDialog(QWidget *parent)
{
    SyncAPISettingsDialog dlg(parent);
    return dlg.execWithSettings() == QDialog::Accepted;
}

bool SqliteSyncPro::authenticateFromSettings()
{
    // Read all sync_api settings.
    QSettings s(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    s.beginGroup(QString::fromLatin1(kSettingsGroup));

    const int     hostType  = s.value(QStringLiteral("syncHostType"), 0).toInt();
    const QString url       = s.value(QStringLiteral("postgrestUrl")).toString().trimmed();
    const QString username  = s.value(QStringLiteral("username")).toString().trimmed();
    const QString password  = settingsDeobfuscate(
                                  s.value(QStringLiteral("password")).toString());
    const QString encPhrase = settingsDeobfuscate(
                                  s.value(QStringLiteral("encryptionPhrase")).toString());
    const QString authKey   = settingsDeobfuscate(
                                  s.value(QStringLiteral("authenticationKey")).toString());

    s.endGroup();

    qDebug() << "[SqliteSyncPro::authenticateFromSettings]"
             << "org="    << kSettingsOrg
             << "app="    << kSettingsApp
             << "group="  << kSettingsGroup
             << "hostType=" << hostType
             << "url="    << url
             << "username=" << username
             << "password set=" << !password.isEmpty()
             << "authKey set="  << !authKey.isEmpty()
             << "encPhrase set=" << !encPhrase.isEmpty();

    if (url.isEmpty() || username.isEmpty()) {
        const QString err = QStringLiteral(
            "Sync settings are incomplete (url='%1' username='%2'). "
            "Open Settings… and fill in the connection details.")
            .arg(url, username);
        qWarning() << "[SqliteSyncPro::authenticateFromSettings]" << err;
        QMutexLocker lock(&m_mutex);
        m_lastError = err;
        return false;
    }

    // Configure encryption key if a passphrase was saved.
    if (!encPhrase.isEmpty()) {
        const QByteArray key32 =
            QCryptographicHash::hash(encPhrase.toUtf8(), QCryptographicHash::Sha256);
        setEncryptionKey(key32);
    } else {
        clearEncryptionKey();
    }

    // Authenticate based on host type.
    if (hostType == 1) {
        // Supabase: the saved URL is the Supabase project base URL.
        // PostgREST is at <url>/rest/v1; auth is at <url>/auth/v1/token.
        const QString postgrestUrl = url + QStringLiteral("/rest/v1");
        qDebug() << "[SqliteSyncPro::authenticateFromSettings] Supabase mode"
                 << "postgrestUrl=" << postgrestUrl;
        return authenticateSupabase(postgrestUrl, url, authKey, username, password);
    } else {
        // Self-hosted: the saved URL is the PostgREST server root.
        qDebug() << "[SqliteSyncPro::authenticateFromSettings] Self-hosted mode"
                 << "url=" << url;
        return authenticate(url, username, password);
    }
}

// ---------------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------------

bool SqliteSyncPro::authenticate(const QString &serverUrl,
                                  const QString &email,
                                  const QString &password,
                                  const QString &loginEndpoint)
{
    // Auth uses a short-lived, local HttpClient – no shared state.
    HttpClient http;
    http.setBaseUrl(serverUrl);

    // Pass the relative endpoint — AuthManager routes it through the client
    // (client->post(endpoint)), which prepends serverUrl automatically.
    QString authError;
    AuthManager auth;
    QObject::connect(&auth, &AuthManager::authenticationFailed,
                     &auth, [&authError](const QString &reason) { authError = reason; });
    if (!auth.login(&http, loginEndpoint, email, password)) {
        QMutexLocker lock(&m_mutex);
        m_lastError = authError.isEmpty() ? QStringLiteral("Authentication failed") : authError;
        qWarning() << "[SqliteSyncPro::authenticate] failed:" << m_lastError;
        return false;
    }

    const QString token = auth.token();
    const QString sub   = jwtSubject(token);

    QMutexLocker lock(&m_mutex);
    m_serverUrl = serverUrl;
    m_authToken = token;
    // Auto-populate userId from the JWT "sub" claim so callers don't have to
    // parse the token themselves.  setUserId() can still override this.
    if (!sub.isEmpty())
        m_userId = sub;
    m_lastError.clear();
    return true;
}

bool SqliteSyncPro::authenticateWithToken(const QString &serverUrl, const QString &jwtToken)
{
    if (jwtToken.trimmed().isEmpty()) {
        QMutexLocker lock(&m_mutex);
        m_lastError = QStringLiteral("Empty token provided");
        return false;
    }

    QMutexLocker lock(&m_mutex);
    m_serverUrl = serverUrl;
    m_authToken = jwtToken;
    m_lastError.clear();
    return true;
}

bool SqliteSyncPro::authenticateSupabase(const QString &serverUrl,
                                          const QString &supabaseUrl,
                                          const QString &supabaseKey,
                                          const QString &email,
                                          const QString &password)
{
    // The auth call goes to the Supabase Auth service (different host/path from
    // PostgREST).  Set the apikey on the client before login() so it is included
    // in the auth POST and in all subsequent PostgREST sync requests.
    HttpClient http;
    http.setBaseUrl(serverUrl);
    http.setApiKey(supabaseKey);

    // Full absolute URL — AuthManager will use its own QNAM for this call.
    const QString authUrl = supabaseUrl.trimmed()
                          + QStringLiteral("/auth/v1/token?grant_type=password");

    QString authError;
    AuthManager auth;
    QObject::connect(&auth, &AuthManager::authenticationFailed,
                     &auth, [&authError](const QString &reason) { authError = reason; });
    if (!auth.login(&http, authUrl, email, password)) {
        QMutexLocker lock(&m_mutex);
        m_lastError = authError.isEmpty()
                          ? QStringLiteral("Supabase authentication failed") : authError;
        qWarning() << "[SqliteSyncPro::authenticateSupabase] failed:" << m_lastError;
        return false;
    }

    const QString token = auth.token();
    const QString sub   = jwtSubject(token);

    QMutexLocker lock(&m_mutex);
    m_serverUrl   = serverUrl;
    m_authToken   = token;
    m_supabaseKey = supabaseKey;
    // Auto-populate userId from the JWT "sub" claim.
    if (!sub.isEmpty())
        m_userId = sub;
    m_lastError.clear();
    return true;
}

// ---------------------------------------------------------------------------
// Local database
// ---------------------------------------------------------------------------

bool SqliteSyncPro::openDatabase(const QString &dbPath)
{
    // Open a temporary connection to validate the path and configure WAL mode.
    // This connection is closed before returning; each sync call opens its own.
    const QString connName = QStringLiteral("SqliteSyncPro_init_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    auto cleanup = [&db, &connName]() {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    };

    if (!db.open()) {
        const QString err = QStringLiteral("Cannot open database '%1': %2")
                                .arg(dbPath, db.lastError().text());
        cleanup();

        QMutexLocker lock(&m_mutex);
        m_lastError = err;
        return false;
    }

    // WAL mode: allows multiple concurrent readers alongside one writer.
    // This persists in the database file so subsequent connections inherit it.
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("PRAGMA journal_mode=WAL"))) {
        qWarning() << "SqliteSyncPro: failed to set WAL mode:" << q.lastError().text();
    }

    cleanup();

    QMutexLocker lock(&m_mutex);
    m_dbPath  = dbPath;
    m_dbOpen  = true;
    m_lastError.clear();
    return true;
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
// Synchronization – internal helper
// ---------------------------------------------------------------------------

/**
 * Opens a thread-local QSqlDatabase connection, runs sync for the given
 * tables using a local HttpClient and SyncEngine, then cleans up.
 *
 * Called from synchronize(), synchronizeTable(), and (indirectly) from
 * SyncWorker::run() which does the same thing independently.
 */
SyncResult SqliteSyncPro::runSync(const QList<SyncTableConfig> &tables)
{
    SyncResult combined;

    // Snapshot config under the lock – release before doing I/O.
    QString dbPath, serverUrl, authToken, supabaseKey, userId, postgresTableName;
    QByteArray encryptionKey;
    QReadWriteLock *dbLock = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_dbOpen) {
            combined.success      = false;
            combined.errorMessage = QStringLiteral("No database open. Call openDatabase() first.");
            m_lastError           = combined.errorMessage;
            return combined;
        }
        if (m_authToken.isEmpty()) {
            combined.success      = false;
            combined.errorMessage = QStringLiteral("Not authenticated. Call authenticate() first.");
            m_lastError           = combined.errorMessage;
            return combined;
        }
        dbPath             = m_dbPath;
        serverUrl          = m_serverUrl;
        authToken          = m_authToken;
        supabaseKey        = m_supabaseKey;
        userId             = m_userId;
        postgresTableName  = m_postgresTableName;
        encryptionKey      = m_encryptionKey;
        dbLock             = m_dbLock;
    }

    // Unique connection name: safe even if this method is called concurrently
    // from multiple threads.
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
        combined.errorMessage = QStringLiteral("Cannot open database in sync thread: %1")
                                    .arg(db.lastError().text());
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);

        QMutexLocker lock(&m_mutex);
        m_lastError = combined.errorMessage;
        return combined;
    }

    // HttpClient must be created in the calling thread so its
    // QNetworkAccessManager has the correct thread affinity.
    HttpClient http;
    http.setBaseUrl(serverUrl);
    http.setAuthToken(authToken);
    // For Supabase: every PostgREST request also needs apikey: <anon_key>.
    // Empty for self-hosted PostgREST (no-op).
    http.setApiKey(supabaseKey);

    SyncEngine engine;
    engine.setDatabase(db);
    engine.setHttpClient(&http);
    engine.setUserId(userId);
    engine.setPostgresTableName(postgresTableName);

    // Forward progress – Qt handles queued vs. direct connection automatically.
    connect(&engine, &SyncEngine::progress,
            this,    &SqliteSyncPro::syncProgress,
            Qt::AutoConnection);

    // Pass the lock into the engine so it can acquire/release around only
    // SQLite operations, never holding it during network I/O.
    engine.setDatabaseLock(dbLock);
    engine.setEncryptionKey(encryptionKey);

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
        if (!combined.success)
            m_lastError = combined.errorMessage;
        else
            m_lastError.clear();
    }

    return combined;
}

// ---------------------------------------------------------------------------
// Synchronization – public
// ---------------------------------------------------------------------------

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
        result.errorMessage = QStringLiteral("Table '%1' not found. Call addTable() first.")
                                  .arg(tableName);
        QMutexLocker lock(&m_mutex);
        m_lastError = result.errorMessage;
        return result;
    }

    const SyncResult result = runSync(match);
    emit syncCompleted(result);
    return result;
}

void SqliteSyncPro::synchronizeAsync()
{
    QString dbPath, serverUrl, authToken, supabaseKey, userId, postgresTableName;
    QByteArray encryptionKey;
    QList<SyncTableConfig> tables;
    QReadWriteLock *dbLock = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        dbPath            = m_dbPath;
        serverUrl         = m_serverUrl;
        authToken         = m_authToken;
        supabaseKey       = m_supabaseKey;
        userId            = m_userId;
        postgresTableName = m_postgresTableName;
        encryptionKey     = m_encryptionKey;
        tables            = m_tables;
        dbLock            = m_dbLock;
    }

    auto *thread = new QThread;
    auto *worker = new SyncWorker(dbPath, serverUrl, authToken, supabaseKey, userId,
                                  postgresTableName, tables, dbLock, encryptionKey);
    worker->moveToThread(thread);

    connect(thread, &QThread::started,   worker, &SyncWorker::run);
    connect(worker, &SyncWorker::finished, this, &SqliteSyncPro::syncCompleted);
    connect(worker, &SyncWorker::progress, this, &SqliteSyncPro::syncProgress);

    // Self-cleanup: worker and thread destroy themselves when the job finishes.
    connect(worker, &SyncWorker::finished, worker, &QObject::deleteLater);
    connect(worker, &SyncWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished,   thread, &QObject::deleteLater);

    thread->start();
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

QString SqliteSyncPro::lastError() const
{
    QMutexLocker lock(&m_mutex);
    return m_lastError;
}
