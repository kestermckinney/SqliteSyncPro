#pragma once

#include <QByteArray>
#include <QObject>
#include <QReadWriteLock>
#include <QSqlDatabase>

#include "syncconfig.h"
#include "syncresult.h"

class HttpClient;
class SchemaInspector;

/**
 * Core synchronization logic between the local SQLite database and a PostgREST server.
 *
 * Postgres storage model
 * ----------------------
 * All client tables are stored in a SINGLE Postgres table (default name: "sync_data").
 * Each row in that table represents one row from any SQLite table, identified by the
 * combination of userid + tablename + id.  The actual column values are stored
 * as a JSON object in the jsonrowdata column.
 *
 *   sync_data columns:
 *     userid      TEXT  – identifies which user owns the row (enforced by RLS)
 *     tablename   TEXT  – name of the originating SQLite table
 *     id          TEXT  – value of the SQLite row's id column
 *     updateddate BIGINT – last-modified UTC timestamp in milliseconds
 *     jsonrowdata JSONB – row serialised as JSON (id, updateddate, and syncdate excluded)
 *
 * This design means the Postgres schema never needs to change when new SQLite tables
 * or columns are added on the client.
 *
 * Sync phases
 * -----------
 * PUSH – uploads local records where syncdate IS NULL or syncdate < updateddate.
 *        The row is serialised (excluding syncdate) into jsonrowdata and POST'd or
 *        PATCH'd.  On success, local syncdate is set equal to updateddate.
 *
 * PULL – fetches sync_data rows for this tablename with updateddate > last pull time.
 *        jsonrowdata is deserialised and upserted into the local SQLite table.
 *        updateddate determines the winner when both sides have changed (newer wins).
 *        Local syncdate is set to the server's updateddate after each pulled row.
 *
 * A small `_sync_meta` table is created automatically in the local database to persist
 * the last-pull timestamp per table.
 */
class SyncEngine : public QObject
{
    Q_OBJECT

public:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine();

    void setDatabase(const QSqlDatabase &db);
    void setHttpClient(HttpClient *client);

    /**
     * Database lock shared with the calling application (and the SqliteSyncPro
     * instance that owns this engine).  The engine acquires a write lock only
     * around SQLite reads/writes, releasing it before every HTTP call so
     * application threads can mutate the database between network round-trips.
     */
    void setDatabaseLock(QReadWriteLock *lock);

    /** The user ID to include in every push payload (userid column). */
    void setUserId(const QString &userId);

    /**
     * Name of the Postgres table that stores all synced data.
     * Must match what PostgREST exposes.  Default: "sync_data".
     */
    void setPostgresTableName(const QString &tableName);

    /**
     * Optional 32-byte AES-256-GCM encryption key.
     * When set, jsonrowdata is encrypted before push and decrypted after pull.
     * Pass an empty QByteArray to disable encryption.
     */
    void setEncryptionKey(const QByteArray &key32);

    SyncResult synchronizeTable(const SyncTableConfig &config);

signals:
    void progress(const QString &tableName, int processed, int total);

private:
    int  pushLocalChanges(const SyncTableConfig &config, TableSyncResult &result);
    int  pullServerChanges(const SyncTableConfig &config, TableSyncResult &result);

    qint64 getLastPullTime(const QString &tableName);
    void   setLastPullTime(const QString &tableName, qint64 utcMs);

    void ensureSyncMetaTable();

    QSqlDatabase     m_db;
    HttpClient      *m_httpClient      = nullptr;
    SchemaInspector *m_schemaInspector = nullptr;
    QReadWriteLock  *m_dbLock          = nullptr;
    QString          m_userId;
    QString          m_postgresTableName = QStringLiteral("sync_data");
    QByteArray       m_encryptionKey;
};
