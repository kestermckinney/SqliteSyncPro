#pragma once

#include <QByteArray>
#include <QObject>
#include <QList>
#include <QMutex>
#include <QReadWriteLock>
#include <QString>

#include "syncconfig.h"
#include "syncresult.h"

class QWidget;

/**
 * SQLiteSyncPro – main public API.
 *
 * Thread safety
 * -------------
 * SqliteSyncPro stores configuration only (path, URL, token, table list).
 * Every sync call creates its own QSqlDatabase connection and HttpClient in
 * the thread where it runs, so the same instance can be used from multiple
 * threads simultaneously.
 *
 * Database access lock (shared with the calling application)
 * ----------------------------------------------------------
 * The sync engine acquires a WRITE lock on the database lock before each
 * table batch and releases it between tables.  The calling application MUST
 * use the same lock around its own database reads and writes so they do not
 * interleave with a sync:
 *
 *   QReadWriteLock *lock = api.databaseLock();
 *
 *   // App reading:
 *   { QReadLocker rl(lock);  db.exec("SELECT ..."); }
 *
 *   // App writing:
 *   { QWriteLocker wl(lock); db.exec("INSERT ..."); }
 *
 * The lock is released between table batches so the app is not blocked for
 * the entire duration of a sync.  Keep batch sizes small for responsiveness.
 *
 * Alternatively, if the application already has a QReadWriteLock, pass it in:
 *
 *   api.setDatabaseLock(&myExistingLock);
 *
 * WAL mode
 * --------
 * The local SQLite file is opened in WAL (Write-Ahead Logging) mode on the
 * first openDatabase() call.  WAL allows concurrent readers alongside one
 * writer at the SQLite engine level.  The QReadWriteLock above provides the
 * additional application-level coordination needed to keep business logic
 * consistent across threads.
 *
 * Typical usage (single-threaded or background thread):
 *
 *   SqliteSyncPro api;
 *   api.authenticate("https://my-postgrest.example.com", "user@example.com", "secret");
 *   api.openDatabase("/path/to/local.db");
 *   api.addTable("invoices",      50);
 *   api.addTable("invoice_lines", 50);
 *
 *   // Blocking – call from a worker thread, not the UI thread:
 *   SyncResult result = api.synchronize();
 *
 *   // Or fire-and-forget – returns immediately:
 *   api.synchronizeAsync();   // listen to syncProgress / syncCompleted
 */
class SqliteSyncPro : public QObject
{
    Q_OBJECT

public:
    explicit SqliteSyncPro(QObject *parent = nullptr);
    ~SqliteSyncPro() override = default;

    // ------------------------------------------------------------------
    // Settings dialog
    // ------------------------------------------------------------------

    /**
     * Open the sync connection settings dialog.
     * The dialog loads existing settings on open and saves them when the
     * user clicks OK.  Returns true if the user accepted.
     *
     * Settings are stored in QSettings("SqliteSyncPro", "SQLSyncAdmin")
     * under the "sync_api" group — the same location used by the
     * SQLSync Administrator app.
     *
     * @param parent  Optional parent widget for dialog centering.
     */
    bool showSettingsDialog(QWidget *parent = nullptr);

    /**
     * Authenticate using the credentials and host type saved in QSettings.
     *
     * Reads syncHostType, postgrestUrl, username, password, authenticationKey,
     * and encryptionPhrase from the "sync_api" settings group and calls the
     * appropriate authenticate method:
     *   - Self-Hosted → authenticate(url, email, password)
     *   - Supabase    → authenticateSupabase(postgrestUrl, supabaseUrl, anonKey,
     *                                        email, password)
     *
     * Also configures the encryption key if an encryptionPhrase was saved.
     * Returns false and sets lastError() if settings are incomplete or auth fails.
     */
    bool authenticateFromSettings();

    // ------------------------------------------------------------------
    // Authentication
    // ------------------------------------------------------------------

    /**
     * Authenticate against a self-hosted PostgREST server.
     * POSTs {"email", "password"} to serverUrl/loginEndpoint and stores the JWT.
     */
    bool authenticate(const QString &serverUrl,
                      const QString &email,
                      const QString &password,
                      const QString &loginEndpoint = QStringLiteral("rpc/rpc_login"));

    bool authenticateWithToken(const QString &serverUrl, const QString &jwtToken);

    /**
     * Authenticate via Supabase Auth (for Supabase-hosted mode).
     *
     * POST <supabaseUrl>/auth/v1/token?grant_type=password
     * Headers: apikey: <supabaseKey>
     * Body: {email, password}
     * Response: {access_token: "jwt…"}
     *
     * The access_token JWT is used as the Bearer token for all PostgREST requests.
     * The "sub" claim is extracted and stored as the userId automatically.
     */
    bool authenticateSupabase(const QString &serverUrl,
                              const QString &supabaseUrl,
                              const QString &supabaseKey,
                              const QString &email,
                              const QString &password);

    /**
     * Set the user ID included in every push payload (stored as USERID in Postgres).
     * Must be called after authentication and before the first sync.
     * The server's Row-Level Security policy uses this value to isolate users.
     */
    void setUserId(const QString &userId);

    /**
     * Override the Postgres table name that stores all synced data.
     * Defaults to "sync_data".  Must match what PostgREST exposes.
     */
    void setPostgresTableName(const QString &tableName);

    // ------------------------------------------------------------------
    // Local database
    // ------------------------------------------------------------------

    /**
     * Validate the database path and enable WAL journal mode.
     * Does NOT keep a persistent connection open; each sync call opens its own.
     */
    bool openDatabase(const QString &dbPath);

    // ------------------------------------------------------------------
    // Database lock – share with the calling application
    // ------------------------------------------------------------------

    /**
     * Returns the QReadWriteLock the sync engine uses around database operations.
     * The calling application MUST use this same lock around its own reads and
     * writes to coordinate with the sync:
     *
     *   QReadLocker  rl(api.databaseLock());  // for SELECT
     *   QWriteLocker wl(api.databaseLock());  // for INSERT / UPDATE / DELETE
     */
    QReadWriteLock *databaseLock() const;

    /**
     * Replace the internal lock with one the application already owns.
     * Ownership is NOT transferred; the lock must outlive this SqliteSyncPro.
     * Call this before any sync starts.
     */
    void setDatabaseLock(QReadWriteLock *lock);

    // ------------------------------------------------------------------
    // Encryption (optional)
    // ------------------------------------------------------------------

    /**
     * Set a 32-byte AES-256-GCM encryption key.  When set, JSONROWDATA is
     * encrypted before being sent to Postgres and decrypted after pull.
     * Derive the key from a user passphrase with QCryptographicHash::Sha256.
     * Pass an empty QByteArray to disable encryption (default).
     */
    void setEncryptionKey(const QByteArray &key32);

    /** Remove the encryption key; subsequent syncs use plain JSONROWDATA. */
    void clearEncryptionKey();

    /** Returns true if an encryption key is currently set. */
    bool hasEncryptionKey() const;

    // ------------------------------------------------------------------
    // Table configuration
    // ------------------------------------------------------------------

    void addTable(const QString &tableName, int batchSize = 100);
    void addTable(const SyncTableConfig &config);
    void clearTables();

    // ------------------------------------------------------------------
    // Synchronization
    // ------------------------------------------------------------------

    /** Synchronize all configured tables in the CALLING thread. Blocks until complete. */
    SyncResult synchronize();

    /** Synchronize a single named table in the CALLING thread. Blocks until complete. */
    SyncResult synchronizeTable(const QString &tableName);

    /**
     * Start an asynchronous sync of all configured tables in a new QThread.
     * Returns immediately. Connect to syncProgress / syncCompleted for results.
     */
    void synchronizeAsync();

    // ------------------------------------------------------------------
    // Status
    // ------------------------------------------------------------------

    bool    isAuthenticated() const;
    bool    isDatabaseOpen()  const;
    QString lastError()       const;

signals:
    void syncProgress(const QString &tableName, int processed, int total);
    void syncCompleted(SyncResult result);

private:
    SyncResult runSync(const QList<SyncTableConfig> &tables);

    mutable QMutex  m_mutex;           // protects config members below
    QString         m_dbPath;
    QString         m_serverUrl;
    QString         m_authToken;
    QString         m_supabaseKey;     // anon key; empty for self-hosted
    QString         m_userId;
    QString         m_postgresTableName = QStringLiteral("sync_data");
    QList<SyncTableConfig> m_tables;
    bool            m_dbOpen    = false;
    QString         m_lastError;

    // Database access lock – shared with the calling application.
    // Points to m_internalLock by default; replaced by setDatabaseLock().
    QReadWriteLock  m_internalLock;
    QReadWriteLock *m_dbLock = &m_internalLock;

    QByteArray      m_encryptionKey;  // empty = no encryption
};
