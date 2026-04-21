// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QByteArray>
#include <QObject>
#include <QList>
#include <QMutex>
#include <QPointer>
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QString>

#include "syncconfig.h"
#include "syncresult.h"

class QThread;
class QWidget;
class SyncLoopWorker;
class SyncStatsWindow;

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
     * Base interval used for network-error backoff (5× this value, capped at 5 min).
     * The background loop determines its normal sleep from the adaptive constants
     * (kCatchUpIntervalMs / kSteadyStateIntervalMs) and ignores this value except
     * during backoff.  Default: 15000 ms.  Must be set before initialize().
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

    /**
     * Show (show == true) or hide (show == false) the network traffic stats window.
     *
     * When shown the window displays a scrolling chart of upload/download KB per
     * sync cycle and a text log with per-table byte and row counts.  The window
     * emits windowClosed() when the user closes it — connect to statsWindowClosed()
     * to keep a menu checkmark in sync.
     *
     * Calling showStats(true) while the window is already visible brings it to
     * the front.  Calling showStats(false) hides (but does not destroy) the window
     * so its history is preserved if shown again.
     */
    void showStats(bool show = true);

    /** Returns true if the stats window is currently visible. */
    bool isStatsVisible() const;

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

    /**
     * Wake the background sync loop from its inter-cycle sleep so it starts
     * a new sync cycle immediately.  Has no effect if the loop is not sleeping
     * (e.g. a cycle is already in progress) or if the loop has stopped.
     * Call this when network connectivity is restored after an outage.
     */
    void retryNow();

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

    /**
     * Force a full resync of all configured tables.
     *
     * Sets syncdate = NULL on every row in every sync table (marking all
     * local records as dirty so they are pushed to the server), and clears
     * the _sync_meta table (resetting all pull high-water marks to 0 so
     * every server record is pulled back down from the beginning).
     *
     * The next sync cycle after this call will push all local records and
     * pull all server records, effectively refreshing both sides.
     *
     * Must be called after initialize() or after addTable() + openDatabase().
     * Returns false if the database is not open or no tables are configured.
     */
    bool syncAll();

    // ------------------------------------------------------------------
    // Status
    // ------------------------------------------------------------------

    bool    isAuthenticated() const;
    bool    isDatabaseOpen()  const;
    bool    isInitialized()   const;
    QString lastError()       const;

    /**
     * Asynchronously compute sync completeness and emit syncStatusUpdated(percent).
     * percent == 100 means no NULL syncdates locally AND the pull is caught up.
     * Pass the SyncResult from the most recent cycle so the worker can apply the
     * same "pulled 0 with no network error → pull is done" rule used in the log.
     * Must be called after initialize().
     */
    void checkSyncStatus(const SyncResult &lastResult);

signals:
    void syncProgress(const QString &tableName, int processed, int total);
    void syncCompleted(SyncResult result);

    /** Emitted when the background sync loop thread has stopped cleanly. */
    void syncStopped();

    /**
     * Emitted when the server returns HTTP 401 and reauthentication with the
     * stored credentials fails.  The sync loop has stopped.  The calling
     * application should prompt the user to verify their credentials via
     * File > Cloud Sync Settings, then re-open the database to resume syncing.
     */
    void authenticationRequired();

    /**
     * Emitted for each row inserted or updated in the local SQLite database
     * during a pull from the server.
     */
    void rowChanged(const QString &tableName, const QString &id);

    /**
     * Emitted by checkSyncStatus() once the background status check completes.
     * percentComplete == 100 means fully synced; < 100 means sync still in progress.
     * pendingPush = local rows with syncdate IS NULL (not yet pushed to server).
     * pendingPull = estimated server rows not yet present locally (0 when caught up).
     */
    void syncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull);

    /**
     * Emitted when the user closes the stats window via its title-bar button.
     * Connect this to uncheck any "Sync Stats" menu action in the host application.
     */
    void statsWindowClosed();

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
    int     m_syncIntervalMs     = 15000;

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

    // Stats window (optional; created on first showStats(true) call)
    QPointer<SyncStatsWindow> m_statsWindow;
};
