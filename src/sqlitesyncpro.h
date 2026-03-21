#pragma once

#include <QByteArray>
#include <QObject>
#include <QList>
#include <QMutex>
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QString>

#include "syncconfig.h"
#include "syncresult.h"

class QThread;
class QWidget;
class SyncLoopWorker;

/**
 * SqliteSyncPro – main public API.
 *
 * Typical usage
 * -------------
 *   SqliteSyncPro api;
 *
 *   // Configure via setters (or showSettingsDialog):
 *   api.setDatabasePath("/path/to/local.db");
 *   api.setPostgrestUrl("https://my-server.example.com");
 *   api.setEmail("user@example.com");
 *   api.setPassword("secret");
 *
 *   // Initialize: opens DB, discovers tables, authenticates, starts background sync.
 *   if (!api.initialize()) { qWarning() << api.lastError(); }
 *
 *   // Use the shared database connection (coordinate with databaseLock()):
 *   QReadLocker rl(api.databaseLock());
 *   QSqlQuery q(api.database());
 *   q.exec("SELECT * FROM invoices");
 *
 *   // React to sync events:
 *   connect(&api, &SqliteSyncPro::rowChanged, this, &MyApp::onRowChanged);
 *
 *   // Shutdown (e.g. on close):
 *   connect(&api, &SqliteSyncPro::syncStopped, this, &QApplication::quit);
 *   api.shutdown();
 *
 * Settings dialog
 * ---------------
 * showSettingsDialog() opens a dialog that reads from and writes to the
 * SqliteSyncPro instance itself (not QSettings).  The calling application is
 * responsible for persisting settings between sessions.
 *
 * Thread safety
 * -------------
 * Configuration members are protected by m_mutex.  The background loop worker
 * runs on its own QThread with its own database connection and HttpClient.
 * The calling application MUST use databaseLock() around its own database
 * reads and writes to coordinate with the background sync.
 */
class SqliteSyncPro : public QObject
{
    Q_OBJECT

public:
    explicit SqliteSyncPro(QObject *parent = nullptr);
    ~SqliteSyncPro() override;

    // ------------------------------------------------------------------
    // Settings – getters and setters (not stored in registry by the API)
    // ------------------------------------------------------------------

    /** Host type: 0 = Self-Hosted PostgREST, 1 = Supabase. */
    void setSyncHostType(int type);
    int  syncHostType() const;

    void    setPostgrestUrl(const QString &url);
    QString postgrestUrl() const;

    void    setEmail(const QString &email);
    QString email() const;

    void    setPassword(const QString &password);
    QString password() const;

    /** Supabase anonymous key (required for Supabase host type). */
    void    setSupabaseKey(const QString &key);
    QString supabaseKey() const;

    /**
     * Optional passphrase for AES-256-GCM row encryption.
     * The API derives a 32-byte key from the passphrase via SHA-256.
     * Set to an empty string to disable encryption.
     */
    void    setEncryptionPhrase(const QString &phrase);
    QString encryptionPhrase() const;

    /**
     * Path to the local SQLite database file.
     * Must be set before calling initialize().
     */
    void    setDatabasePath(const QString &path);
    QString databasePath() const;

    /**
     * Override the Postgres table name that stores all synced data.
     * Defaults to "sync_data".  Must match what PostgREST exposes.
     */
    void    setPostgresTableName(const QString &tableName);
    QString postgresTableName() const;

    /**
     * Interval between background sync rounds in milliseconds.
     * Default: 5000 ms.  Must be set before initialize().
     */
    void setSyncIntervalMs(int ms);
    int  syncIntervalMs() const;

    // ------------------------------------------------------------------
    // Settings dialog
    // ------------------------------------------------------------------

    /**
     * Open the sync connection settings dialog.
     * The dialog reads current settings from this instance and writes back
     * to this instance on accept.  The calling application is responsible
     * for persisting settings (e.g. to QSettings) after the dialog closes.
     * Returns true if the user accepted.
     */
    bool showSettingsDialog(QWidget *parent = nullptr);

    // ------------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------------

    /**
     * Initialize the API:
     *  1. Opens the database (databasePath must be set).
     *  2. Authenticates using the current credential settings.
     *  3. Inspects all tables in the database; adds those that have the
     *     required sync columns (id, updateddate, syncdate).  Tables
     *     missing required columns are silently skipped.
     *  4. Keeps a persistent QSqlDatabase open for the calling application.
     *  5. Starts the background sync loop thread.
     *
     * Returns false on error; call lastError() for details.
     * Calling initialize() while already initialized calls shutdown() first.
     */
    bool initialize();

    // ------------------------------------------------------------------
    // Database access (available after initialize())
    // ------------------------------------------------------------------

    /**
     * Returns the persistent QSqlDatabase connection opened during initialize().
     * The calling application may use this connection for its own queries.
     * Always coordinate with databaseLock() around reads and writes.
     */
    QSqlDatabase database() const;

    /**
     * Returns the QReadWriteLock shared between the sync engine and the
     * calling application.  Use QReadLocker for SELECT queries and
     * QWriteLocker for INSERT / UPDATE / DELETE.
     */
    QReadWriteLock *databaseLock() const;

    // ------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------

    /**
     * Request the background sync loop to stop.
     * Returns immediately; the syncStopped() signal is emitted once the
     * background thread has exited cleanly.
     */
    void shutdown();

    // ------------------------------------------------------------------
    // Backwards-compatible helpers (kept for existing code and tests)
    // ------------------------------------------------------------------

    /**
     * Set a JWT auth token directly without an HTTP authentication call.
     * Useful for testing and for systems that obtain tokens externally.
     */
    bool authenticateWithToken(const QString &serverUrl, const QString &jwtToken);

    /**
     * Validate the database path and enable WAL journal mode.
     * Equivalent to setDatabasePath(dbPath) followed by a validation open.
     */
    bool openDatabase(const QString &dbPath);

    /**
     * Replace the internal lock with one the application already owns.
     * Ownership is NOT transferred; the lock must outlive this object.
     * Call before any sync starts.
     */
    void setDatabaseLock(QReadWriteLock *lock);

    /**
     * Start a one-shot asynchronous sync of all configured tables.
     * Returns immediately; listen to syncProgress / syncCompleted for results.
     */
    void synchronizeAsync();

    // ------------------------------------------------------------------
    // Encryption (optional, alternative to setEncryptionPhrase)
    // ------------------------------------------------------------------

    /** Set a raw 32-byte AES-256-GCM key directly. */
    void       setEncryptionKey(const QByteArray &key32);
    void       clearEncryptionKey();
    bool       hasEncryptionKey() const;

    // ------------------------------------------------------------------
    // Manual table configuration (used by synchronize / synchronizeTable)
    // ------------------------------------------------------------------

    void addTable(const QString &tableName, int batchSize = 100);
    void addTable(const SyncTableConfig &config);
    void clearTables();

    // ------------------------------------------------------------------
    // Manual synchronization (blocking – for use outside the loop)
    // ------------------------------------------------------------------

    /** Synchronize all configured tables in the CALLING thread. */
    SyncResult synchronize();

    /** Synchronize a single named table in the CALLING thread. */
    SyncResult synchronizeTable(const QString &tableName);

    // ------------------------------------------------------------------
    // Status
    // ------------------------------------------------------------------

    bool    isAuthenticated() const;
    bool    isDatabaseOpen()  const;
    bool    isInitialized()   const;
    QString lastError()       const;

signals:
    void syncProgress(const QString &tableName, int processed, int total);
    void syncCompleted(SyncResult result);

    /** Emitted when the background sync loop thread has stopped cleanly. */
    void syncStopped();

    /**
     * Emitted for each row inserted or updated in the local SQLite database
     * during a pull from the server.
     */
    void rowChanged(const QString &tableName, const QString &id);

private:
    bool    doAuthenticate();
    SyncResult runSync(const QList<SyncTableConfig> &tables);
    QStringList discoverSyncTables(const QString &dbPath);

    mutable QMutex m_mutex;

    // Settings (not stored in registry by the API)
    int     m_syncHostType       = 0;
    QString m_postgrestUrl;
    QString m_email;
    QString m_password;
    QString m_supabaseKey;
    QString m_encryptionPhrase;
    QString m_databasePath;
    QString m_postgresTableName  = QStringLiteral("sync_data");
    int     m_syncIntervalMs     = 5000;

    // Runtime state (set during authenticate/initialize)
    QString m_authToken;
    QString m_userId;
    bool    m_dbOpen       = false;
    bool    m_initialized  = false;
    QString m_lastError;

    // Persistent database connection (held for the calling application)
    QSqlDatabase   m_persistentDb;
    QString        m_persistentConnName;

    // Database access lock – shared with the calling application and the sync engine.
    // m_dbLock points to m_internalLock by default; setDatabaseLock() can redirect it.
    QReadWriteLock  m_internalLock;
    QReadWriteLock *m_dbLock = &m_internalLock;

    // Encryption
    QByteArray m_encryptionKey;

    // Manually-configured tables (used by synchronize / synchronizeTable).
    // When initialize() discovers tables automatically these are replaced.
    QList<SyncTableConfig> m_tables;

    // Background loop
    QThread         *m_syncThread = nullptr;
    SyncLoopWorker  *m_syncWorker = nullptr;
};
