// Copyright (C) 2026 Paul McKinney
#include "syncengine.h"
#include "httpclient.h"
#include "rowencryption.h"
#include "schemainspector.h"

#include <QReadWriteLock>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

#if 0 // QT_DEBUG
static QString debugQuery(const QSqlQuery &q)
{
    QString sql = q.lastQuery();
    const QVariantList vals = q.boundValues();
    if (!vals.isEmpty()) {
        QStringList parts;
        for (int i = 0; i < vals.size(); ++i)
            parts << QStringLiteral("%1=%2").arg(q.boundValueName(i), vals[i].toString());
        sql += QStringLiteral(" [bindings: %1]").arg(parts.join(QStringLiteral(", ")));
    }
    return sql;
}
#endif

// Helper: wrap a value in double quotes if it contains characters that are
// special in postgREST filter syntax.  This forces postgREST to treat
// the value as a literal string.
static QString postgrestFilterValue(const QString &value)
{
    // postgREST special characters in filter syntax
    if (value.contains(QLatin1Char('{')) || value.contains(QLatin1Char('}')) ||
        value.contains(QLatin1Char('(')) || value.contains(QLatin1Char(')')) ||
        value.contains(QLatin1Char(',')) || value.contains(QLatin1Char('=')) ||
        value.contains(QLatin1Char('>')) || value.contains(QLatin1Char('<')) ||
        value.contains(QLatin1Char('"')))
    {
        return QStringLiteral("\"%1\"").arg(value);
    }
    return value;
}

SyncEngine::SyncEngine(QObject *parent)
    : QObject(parent)
{
}

SyncEngine::~SyncEngine()
{
    delete m_schemaInspector;
}

void SyncEngine::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
    delete m_schemaInspector;
    m_schemaInspector = new SchemaInspector(m_db);
    ensureSyncMetaTable();
}

void SyncEngine::setHttpClient(HttpClient *client)
{
    m_httpClient = client;
}

void SyncEngine::setDatabaseLock(QReadWriteLock *lock)
{
    m_dbLock = lock;
}

void SyncEngine::setUserId(const QString &userId)
{
    m_userId = userId;
}

void SyncEngine::setPostgresTableName(const QString &tableName)
{
    m_postgresTableName = tableName;
}

void SyncEngine::setEncryptionKey(const QByteArray &key32)
{
    m_encryptionKey = key32;
}

// ---------------------------------------------------------------------------
// _sync_meta helpers
// ---------------------------------------------------------------------------

void SyncEngine::ensureSyncMetaTable()
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS _sync_meta ("
        "  table_name     TEXT    PRIMARY KEY,"
        "  last_pull_time INTEGER NOT NULL DEFAULT 0,"
        "  last_pull_id   TEXT    NOT NULL DEFAULT ''"
        ")"
    ));

    // Migrate older databases that predate the composite cursor.
    QSqlQuery infoQ(m_db);
    if (infoQ.exec(QStringLiteral("PRAGMA table_info(_sync_meta)"))) {
        bool hasLastPullId = false;
        while (infoQ.next()) {
            if (infoQ.value(1).toString() == QLatin1String("last_pull_id")) {
                hasLastPullId = true;
                break;
            }
        }
        if (!hasLastPullId) {
            QSqlQuery alterQ(m_db);
            alterQ.exec(QStringLiteral(
                "ALTER TABLE _sync_meta ADD COLUMN last_pull_id TEXT NOT NULL DEFAULT ''"
            ));
        }
    }
}

SyncEngine::PullCursor SyncEngine::getLastPullCursor(const QString &tableName)
{
    PullCursor cursor;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT last_pull_time, last_pull_id FROM _sync_meta WHERE table_name = :name"
    ));
    q.bindValue(QStringLiteral(":name"), tableName);
    if (q.exec() && q.next()) {
        cursor.ts = q.value(0).toLongLong();
        cursor.id = q.value(1).toString();
    }
    return cursor;
}

void SyncEngine::setLastPullCursor(const QString &tableName, qint64 ts, const QString &id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO _sync_meta (table_name, last_pull_time, last_pull_id) "
        "VALUES (:name, :ts, :id)"
    ));
    q.bindValue(QStringLiteral(":name"), tableName);
    q.bindValue(QStringLiteral(":ts"),   ts);
    q.bindValue(QStringLiteral(":id"),   id);
    if (!q.exec()) {
#if 0 // QT_DEBUG
        qWarning() << "Failed to set last pull cursor for table" << tableName
                   << ":" << q.lastError().text();
        qWarning().noquote() << "[SyncEngine] Failed SQL:" << debugQuery(q);
#endif
    }
}

qint64 SyncEngine::getLastPullTime(const QString &tableName)
{
    return getLastPullCursor(tableName).ts;
}

void SyncEngine::setLastPullTime(const QString &tableName, qint64 utcMs)
{
    // Clearing the id is intentional — see header doc.
    setLastPullCursor(tableName, utcMs, QString());
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

SyncResult SyncEngine::synchronizeTable(const SyncTableConfig &config)
{
    SyncResult      result;
    TableSyncResult tableResult;
    tableResult.tableName = config.tableName;

    if (!m_httpClient || !m_schemaInspector) {
        tableResult.success      = false;
        tableResult.errorMessage = QStringLiteral("SyncEngine not initialised");
        result.success           = false;
        result.errorMessage      = tableResult.errorMessage;
        result.tableResults.append(tableResult);
        return result;
    }

    if (m_postgresTableName.isEmpty()) {
        tableResult.success      = false;
        tableResult.errorMessage = QStringLiteral("Postgres table name not set");
        result.success           = false;
        result.errorMessage      = tableResult.errorMessage;
        result.tableResults.append(tableResult);
        return result;
    }

    QString colError;
    if (!m_schemaInspector->hasRequiredColumns(config.tableName,
                                               config.idColumn,
                                               config.updatedDateColumn,
                                               config.syncDateColumn,
                                               colError)) {
        tableResult.success      = false;
        tableResult.errorMessage = colError;
        result.success           = false;
        result.errorMessage      = colError;
        result.tableResults.append(tableResult);
        return result;
    }

    // Pull before push: this validates that decryption works with the current
    // encryption phrase before we overwrite any server data.  If decryption
    // fails here it means the phrase is wrong; we skip the push so the server
    // data (encrypted with the previous correct phrase) is not corrupted.
    const int pulled = pullServerChanges(config, tableResult);
    if (pulled < 0) {
        tableResult.success = false;
        result.success      = false;
        result.tableResults.append(tableResult);
        return result;
    }
    tableResult.pulled = pulled;

    if (tableResult.decryptionFailures > 0) {
        tableResult.success      = false;
        tableResult.errorMessage = QStringLiteral("Decryption failed: encryption phrase may be incorrect");
        result.success           = false;
        result.errorMessage      = tableResult.errorMessage;
        result.tableResults.append(tableResult);
        return result;
    }

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Synchronize '%1': pull complete (pulled=%2). Starting push...")
                              .arg(config.tableName).arg(pulled);
#endif

    const int pushed = pushLocalChanges(config, tableResult);
    if (pushed < 0) {
        tableResult.success = false;
        result.success      = false;
        result.tableResults.append(tableResult);
        return result;
    }
    tableResult.pushed = pushed;

    // NOTE: we intentionally do NOT reset lastPullTime to 0 after a push.
    // Resetting to 0 caused an infinite loop: while the push phase keeps
    // finding records to send, the pull phase is forced to restart from the
    // beginning every cycle and can never advance past the first batch.
    // The pull cursor on (server_modified_at, id) already re-fetches the
    // high-water-mark boundary row on each cycle when its id matches lastId,
    // so no additional safety margin is needed.
    // if (pushed > 0) {
    //     qDebug().noquote() << QStringLiteral("[SyncEngine] '%1': pushed %2 record(s)")
    //                               .arg(config.tableName).arg(pushed);
    // }

    result.tableResults.append(tableResult);
    return result;
}

// ---------------------------------------------------------------------------
// PUSH phase – send local changes to the Postgres sync_data table
//
// Postgres row shape:
//   { userid, tablename, id, updateddate, jsonrowdata:{…} }
//
// jsonrowdata holds every SQLite column except syncdate.
//
// Batched locking and HTTP strategy (replaces the old per-record approach):
//   1. Acquire write lock → SELECT all dirty records into a buffer → release lock
//   2. ONE batch GET to fetch server timestamps for all pending IDs (no lock held)
//   3. Categorise: upsert candidates vs. conflicts vs. equal-timestamp
//   4. ONE batch POST upsert for all candidates (no lock held)
//   5. Acquire write lock → bulk UPDATE syncdate for all stamped records → release lock
// ---------------------------------------------------------------------------

int SyncEngine::pushLocalChanges(const SyncTableConfig &config, TableSyncResult &tableResult)
{
#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': entered push phase. syncDateColumn=%2, updatedDateColumn=%3, batchSize=%4")
                              .arg(config.tableName, config.syncDateColumn, config.updatedDateColumn)
                              .arg(config.batchSize);
#endif

    struct PendingRecord {
        QString     id;
        qint64      ts;
        QJsonObject rowJson;
    };

    const auto columns = m_schemaInspector->getColumns(config.tableName);
    if (columns.isEmpty())
        return 0;

    // --- Step 1: read all dirty records into memory while holding the write lock ---
    QList<PendingRecord> pending;
    {
        if (m_dbLock) m_dbLock->lockForWrite();

        const QString selectSql = QStringLiteral(
            "SELECT * FROM \"%1\" WHERE \"%2\" IS NULL OR \"%2\" < \"%3\" LIMIT %4"
        ).arg(config.tableName, config.syncDateColumn, config.updatedDateColumn)
         .arg(config.batchSize);

#if 0 // QT_DEBUG
        qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': pending-records query: %2")
                                  .arg(config.tableName, selectSql);
#endif

        QSqlQuery selectQ(m_db);
        if (!selectQ.exec(selectSql)) {
            if (m_dbLock) m_dbLock->unlock();
            tableResult.errorMessage = QStringLiteral("Failed to query local records: %1")
                                           .arg(selectQ.lastError().text());
#if 0 // QT_DEBUG
            qWarning().noquote() << "[SyncEngine] Failed SQL:" << selectSql;
#endif
            return -1;
        }

        while (selectQ.next()) {
            const QSqlRecord record   = selectQ.record();
            PendingRecord    pr;
            pr.id      = record.value(config.idColumn).toString();
            pr.ts      = record.value(config.updatedDateColumn).toLongLong();
            pr.rowJson = m_schemaInspector->recordToJson(
                record, columns,
                {config.syncDateColumn, config.idColumn, config.updatedDateColumn});
             pending.append(pr);
         }

#if 0 // QT_DEBUG
        qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': pending-records query returned %2 row(s)")
                                  .arg(config.tableName).arg(pending.size());
#endif

         if (m_dbLock) m_dbLock->unlock();
    }

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': %2 pending record(s) found after query")
                              .arg(config.tableName).arg(pending.size());
#endif

    if (pending.isEmpty()) {
#if 0 // QT_DEBUG
        qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': no pending records, exiting push phase")
                              .arg(config.tableName);
#endif
        return 0;
    }

    // Write lock is now released; HTTP calls follow with no lock held.

    // --- Step 2: ONE batch GET to retrieve server timestamps for all pending IDs ---
    QStringList idList;
    idList.reserve(pending.size());
    for (const auto &pr : pending)
        idList << pr.id;

    QUrlQuery checkQuery;
    checkQuery.addQueryItem(QStringLiteral("tablename"),
                            QStringLiteral("eq.%1").arg(config.tableName));

    // For the id list with possible {} characters
    QStringList safeIdList;
    for (const auto &id : std::as_const(idList))
        safeIdList << postgrestFilterValue(id);
    QString idListStr = safeIdList.join(QLatin1Char(','));

    // Properly encode the entire value for PostgREST
    QString encodedIdList = QStringLiteral("in.(%1)").arg(idListStr);
    checkQuery.addQueryItem(QStringLiteral("id"), encodedIdList);

    checkQuery.addQueryItem(QStringLiteral("select"), QStringLiteral("id,updateddate"));

    const QByteArray checkResp = m_httpClient->get(m_postgresTableName, checkQuery);
    tableResult.bytesPulled += m_httpClient->lastBytesReceived();
    tableResult.bytesPushed += m_httpClient->lastBytesSent();

    if (!m_httpClient->wasSuccessful()) {
        tableResult.errorMessage = QStringLiteral("Server batch check failed: %1")
                                       .arg(m_httpClient->lastError());
        if (m_httpClient->lastStatusCode() == 0) {
            tableResult.networkError = true;
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] Network error on batch push check ('%1'): server unreachable (status 0)")
                                        .arg(config.tableName);
#endif
        } else if (m_httpClient->lastStatusCode() == 401) {
            tableResult.authError = true;
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] 401 Unauthorized on batch push check ('%1'): JWT may have expired")
                                        .arg(config.tableName);
#endif
        } else {
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] Batch push check failed ('%1'): HTTP %2 — %3")
                                        .arg(config.tableName)
                                        .arg(m_httpClient->lastStatusCode())
                                        .arg(m_httpClient->lastError());
#endif
        }
        return -1;
    }

    // Build id → serverTs map from the batch response.
    QHash<QString, qint64> serverTimestamps;
    const QJsonArray checkArr = QJsonDocument::fromJson(checkResp).array();
    for (const QJsonValue &v : checkArr) {
        const QJsonObject obj = v.toObject();
        serverTimestamps.insert(
            obj.value(QStringLiteral("id")).toString(),
            obj.value(QStringLiteral("updateddate")).toVariant().toLongLong());
    }

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': server returned %2 existing record(s)")
                              .arg(config.tableName).arg(serverTimestamps.size());
#endif

    // --- Step 3: categorise records and build batch upsert payload ---
    QJsonArray   upsertBatch;
    QList<QString> toStampIds;    // uploaded records that need SYNCDATE stamped
    QList<QString> equalIds;      // equal-timestamp records that just need SYNCDATE stamped
    QList<QString> conflictIds;   // conflict records stamped locally but NOT counted as pushed

    for (const PendingRecord &pr : pending) {
        const bool   onServer = serverTimestamps.contains(pr.id);
        const qint64 srvTs    = onServer ? serverTimestamps.value(pr.id) : -1;

        if (onServer && srvTs > pr.ts) {
            // Server is newer → conflict; server wins.
            // Stamp syncdate so this record leaves the dirty queue; without this
            // stamp a batch of all-conflicts loops endlessly on the same records.
            // The server's authoritative version will be picked up by the next
            // pull cycle on its own — server_modified_at is monotonic so the
            // cursor cannot skip past it.
            ++tableResult.conflicts;
            conflictIds << pr.id;
#if 0 // QT_DEBUG
            qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': conflict id=%2 (server ts=%3 > local ts=%4) — server wins")
                                      .arg(config.tableName, pr.id).arg(srvTs).arg(pr.ts);
#endif
            continue;
        }

        if (onServer && srvTs == pr.ts) {
            // Equal timestamps → already in sync; just stamp SYNCDATE locally
            equalIds << pr.id;
#if 0 // QT_DEBUG
            qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': equal timestamp id=%2 (ts=%3) — marking in sync")
                                      .arg(config.tableName, pr.id).arg(pr.ts);
#endif
            continue;
        }

        // New record or local is newer → include in upsert batch
        QJsonValue jsonRowDataValue;
        if (!m_encryptionKey.isEmpty()) {
            const QByteArray plain = QJsonDocument(pr.rowJson).toJson(QJsonDocument::Compact);
            const QString enc = RowEncryption::encrypt(plain, m_encryptionKey);
            if (enc.isEmpty()) {
#if 0 // QT_DEBUG
                qWarning() << "Encryption failed for record" << pr.id << "– skipping";
#endif
                continue;
            }
            jsonRowDataValue = enc;
        } else {
            jsonRowDataValue = pr.rowJson;
        }

        QJsonObject payload;
        if (!m_userId.isEmpty())
            payload[QStringLiteral("userid")]      = m_userId;
        payload[QStringLiteral("tablename")]   = config.tableName;
        payload[QStringLiteral("id")]          = pr.id;
        payload[QStringLiteral("updateddate")] = pr.ts;
        payload[QStringLiteral("jsonrowdata")] = jsonRowDataValue;

        upsertBatch.append(payload);
        toStampIds << pr.id;

#if 0 // QT_DEBUG
        qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1': upsert id=%2 (local ts=%3, server ts=%4)")
                                  .arg(config.tableName, pr.id).arg(pr.ts).arg(srvTs);
#endif
    }

    // --- Step 4: ONE batch upsert POST for all candidates ---
    if (!upsertBatch.isEmpty()) {
        m_httpClient->post(m_postgresTableName,
                           QJsonDocument(upsertBatch).toJson(QJsonDocument::Compact),
                           {QStringLiteral("resolution=merge-duplicates"),
                            QStringLiteral("return=minimal")});
        tableResult.bytesPushed += m_httpClient->lastBytesSent();
        tableResult.bytesPulled += m_httpClient->lastBytesReceived();
        if (!m_httpClient->wasSuccessful()) {
            tableResult.errorMessage = QStringLiteral("Batch upsert failed: %1")
                                           .arg(m_httpClient->lastError());
            if (m_httpClient->lastStatusCode() == 0)
                tableResult.networkError = true;
            else if (m_httpClient->lastStatusCode() == 401)
                tableResult.authError = true;
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] Batch upsert failed ('%1'): HTTP %2 — %3")
                                        .arg(config.tableName)
                                        .arg(m_httpClient->lastStatusCode())
                                        .arg(m_httpClient->lastError());
#endif
            return -1;
        }
    }

    // --- Step 5: bulk UPDATE syncdate for all stamped records under one write lock ---
    const QList<QString> allToStamp = toStampIds + equalIds;
    int pushed = toStampIds.size();

    if (!allToStamp.isEmpty()) {
        QStringList placeholders;
        placeholders.reserve(allToStamp.size());
        for (int i = 0; i < allToStamp.size(); ++i)
            placeholders << QStringLiteral(":id%1").arg(i);

        QSqlQuery stampQ(m_db);
        stampQ.prepare(QStringLiteral(
            "UPDATE \"%1\" SET \"%2\" = \"%3\" WHERE \"%4\" IN (%5)"
        ).arg(config.tableName, config.syncDateColumn,
              config.updatedDateColumn, config.idColumn,
              placeholders.join(QStringLiteral(", "))));

        for (int i = 0; i < allToStamp.size(); ++i)
            stampQ.bindValue(QStringLiteral(":id%1").arg(i), allToStamp[i]);

        if (m_dbLock) m_dbLock->lockForWrite();
        if (stampQ.exec()) {
            // pushed count already set to toStampIds.size() above
        }
#if 0 // QT_DEBUG
        else {
            qWarning() << "Bulk SYNCDATE stamp failed for table" << config.tableName
                       << stampQ.lastError().text();
            qWarning().noquote() << "[SyncEngine] Failed SQL:" << debugQuery(stampQ);
        }
#endif
        if (m_dbLock) m_dbLock->unlock();

        if (pushed > 0)
            emit progress(config.tableName, pushed, -1);
    }

    // --- Step 5b: stamp syncdate for conflict records (not counted as pushed) ---
    // This removes them from the dirty queue so the same records do not cycle
    // endlessly when an entire batch consists of conflicts.
    if (!conflictIds.isEmpty()) {
        QStringList placeholders;
        placeholders.reserve(conflictIds.size());
        for (int i = 0; i < conflictIds.size(); ++i)
            placeholders << QStringLiteral(":cid%1").arg(i);

        QSqlQuery conflictStampQ(m_db);
        conflictStampQ.prepare(QStringLiteral(
            "UPDATE \"%1\" SET \"%2\" = \"%3\" WHERE \"%4\" IN (%5)"
        ).arg(config.tableName, config.syncDateColumn,
              config.updatedDateColumn, config.idColumn,
              placeholders.join(QStringLiteral(", "))));

        for (int i = 0; i < conflictIds.size(); ++i)
            conflictStampQ.bindValue(QStringLiteral(":cid%1").arg(i), conflictIds[i]);

        if (m_dbLock) m_dbLock->lockForWrite();
        conflictStampQ.exec();
        if (m_dbLock) m_dbLock->unlock();
    }

    // No watermark rollback needed: the pull cursor is keyed off the server-
    // assigned server_modified_at field, which is monotonic across all clients
    // regardless of clock skew or backdated updateddate values.

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Push '%1' summary: pushed=%2, conflicts=%3, equalTs=%4, upsertBatch=%5")
                              .arg(config.tableName).arg(pushed)
                              .arg(conflictIds.size()).arg(equalIds.size())
                              .arg(upsertBatch.size());
    if (!conflictIds.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[SyncEngine] Push '%1': %2 conflict(s) (server wins): %4")
                                     .arg(config.tableName).arg(conflictIds.size())
                                     .arg(conflictIds.mid(0, 10).join(QStringLiteral(", ")))
                                     << (conflictIds.size() > 10 ? QStringLiteral("… (+%1 more)").arg(conflictIds.size() - 10) : QString());
    }
#endif

    return pushed;
}

// ---------------------------------------------------------------------------
// Unique-key conflict resolution helper
//
// Iterates all non-PK unique indexes on the table and returns the local id of
// the first row whose unique key values match those in rowData.  Called while
// the write lock is held, so it may safely query the database.
// ---------------------------------------------------------------------------

QString SyncEngine::findConflictingLocalId(const SyncTableConfig        &config,
                                            const QJsonObject             &rowData,
                                            const QHash<QString, QString> &typeMap)
{
    const auto uniqueIndexes = m_schemaInspector->getUniqueIndexes(config.tableName);
    for (const auto &idx : uniqueIndexes) {
        bool allPresent = true;
        for (const QString &col : idx.columns) {
            if (!rowData.contains(col)) { allPresent = false; break; }
        }
        if (!allPresent)
            continue;

        QStringList whereClauses;
        for (const QString &col : idx.columns)
            whereClauses << QStringLiteral("\"%1\" = :%1").arg(col);

        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT \"%1\" FROM \"%2\" WHERE %3")
                      .arg(config.idColumn, config.tableName,
                           whereClauses.join(QStringLiteral(" AND "))));

        for (const QString &col : idx.columns) {
            q.bindValue(QStringLiteral(":%1").arg(col),
                        SchemaInspector::jsonValueToVariant(
                            rowData.value(col), typeMap.value(col)));
        }

        if (q.exec() && q.next())
            return q.value(0).toString();
    }
    return {};
}

// ---------------------------------------------------------------------------
// PULL phase – fetch server changes and upsert into the local SQLite table
//
// The server returns rows from sync_data; the actual column values are in
// JSONROWDATA.  We deserialise that object and upsert it into the local table.
//
// Fine-grained locking strategy:
//   1. HTTP GET for the batch with NO lock held
//   2. For each server record: acquire write lock → EXISTS check + INSERT or UPDATE → release
//   3. setLastPullTime call at the end: acquire write lock → update → release
// ---------------------------------------------------------------------------

int SyncEngine::pullServerChanges(const SyncTableConfig &config, TableSyncResult &tableResult)
{
    const auto columns = m_schemaInspector->getColumns(config.tableName);
    if (columns.isEmpty())
        return 0;

    QHash<QString, QString> typeMap;
    for (const auto &col : columns)
        typeMap.insert(col.name, col.type);

    const PullCursor cursor   = getLastPullCursor(config.tableName);
    const qint64     lastPull = cursor.ts;
    const QString    lastId   = cursor.id;

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Pull '%1': cursor=(%2, '%3'), batchSize=%4")
                              .arg(config.tableName).arg(lastPull).arg(lastId)
                              .arg(config.batchSize);
#endif

    // --- Step 1: HTTP GET with no lock held ---
    //
    // Composite cursor pagination on (server_modified_at, id):
    //   ORDER BY server_modified_at ASC, id ASC
    //   WHERE (server_modified_at > lastPull)
    //      OR (server_modified_at == lastPull AND id > lastId)
    //
    // server_modified_at is populated by a Postgres trigger on every
    // INSERT/UPDATE so the cursor is monotonic across all clients regardless
    // of clock skew or backdated `updateddate` values. The (ts, id) composite
    // prevents stalls when many rows share the same server_modified_at value;
    // when lastId is empty we fall back to gte semantics so no rows are
    // missed at the boundary.
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("tablename"),
                       QStringLiteral("eq.%1").arg(config.tableName));
    if (!lastId.isEmpty()) {
        const QString safeId = postgrestFilterValue(lastId);
        query.addQueryItem(
            QStringLiteral("or"),
            QStringLiteral("(server_modified_at.gt.%1,and(server_modified_at.eq.%1,id.gt.%2))")
                .arg(lastPull).arg(safeId));
    } else {
        query.addQueryItem(QStringLiteral("server_modified_at"),
                           QStringLiteral("gte.%1").arg(lastPull));
    }
    query.addQueryItem(QStringLiteral("order"),
                       QStringLiteral("server_modified_at.asc,id.asc"));
    query.addQueryItem(QStringLiteral("limit"), QString::number(config.batchSize));

    const QByteArray response = m_httpClient->get(m_postgresTableName, query);
    tableResult.bytesPulled += m_httpClient->lastBytesReceived();
    tableResult.bytesPushed += m_httpClient->lastBytesSent();
    if (!m_httpClient->wasSuccessful()) {
        tableResult.errorMessage = QStringLiteral("Pull request failed: %1")
                                       .arg(m_httpClient->lastError());
        if (m_httpClient->lastStatusCode() == 0) {
            tableResult.networkError = true;
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] Network error pulling '%1': server unreachable (status 0)")
                                        .arg(config.tableName);
#endif
        } else if (m_httpClient->lastStatusCode() == 401) {
            tableResult.authError = true;
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] 401 Unauthorized pulling '%1': JWT may have expired")
                                        .arg(config.tableName);
#endif
        } else {
#if 0 // QT_DEBUG
            qWarning().noquote() << QStringLiteral("[SyncEngine] Pull failed for '%1': HTTP %2 — %3")
                                        .arg(config.tableName)
                                        .arg(m_httpClient->lastStatusCode())
                                        .arg(m_httpClient->lastError());
#endif
        }
        return -1;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isArray()) {
        tableResult.errorMessage = QStringLiteral("Unexpected response format from server during pull");
        return -1;
    }

    const QJsonArray serverRows = doc.array();

#if 0 // QT_DEBUG
    if (!serverRows.isEmpty()) {
        const QJsonObject firstRow = serverRows.first().toObject();
        qDebug().noquote() << QStringLiteral("[SyncEngine] Pull '%1': first row raw JSON:\n%2")
                                  .arg(config.tableName,
                                       QString::fromUtf8(QJsonDocument(firstRow).toJson(QJsonDocument::Indented)));
    }
#endif
    int         pulled       = 0;
    int         successCount = 0;
    qint64      maxSuccessSmta = lastPull; // tracks server_modified_at, drives cursor
    QString     maxSuccessId   = lastId;  // tracks id corresponding to maxSuccessSmta
    qint64      maxBatchSmta   = lastPull; // highest server_modified_at in batch
    QStringList failedIds;

    auto markFailure = [&failedIds](const QString &id) {
        failedIds << id;
    };

#if 0 // QT_DEBUG
    qDebug().noquote() << QStringLiteral("[SyncEngine] Pull '%1': server returned %2 row(s)%3")
                              .arg(config.tableName)
                              .arg(serverRows.count())
                              .arg(serverRows.count() == config.batchSize
                                       ? QStringLiteral(" (batch limit hit — more may remain)")
                                       : QString());
#endif

    // --- Step 2: upsert each record under a per-record write lock ---
    for (const QJsonValue &val : serverRows) {
        if (!val.isObject())
            continue;

        const QJsonObject serverRow = val.toObject();
        // serverTs is the user's business modification date — used to stamp
        // the local row's updateddate/syncdate columns and to compare against
        // any local copy of the row.
        const qint64      serverTs  = serverRow.value(QStringLiteral("updateddate"))
                                          .toVariant().toLongLong();
        // serverSmta is the server-assigned monotonic stamp — used only to
        // advance the pull cursor.
        const qint64      serverSmta = serverRow.value(QStringLiteral("server_modified_at"))
                                          .toVariant().toLongLong();
        const QString     recordId  = serverRow.value(QStringLiteral("id")).toString();

        // jsonrowdata is either a plain JSON object or an encrypted string ("enc:…").
        QJsonObject rowData;
        const QJsonValue jrdVal = serverRow.value(QStringLiteral("jsonrowdata"));
        if (jrdVal.isString()) {
            const QString encoded = jrdVal.toString();
            if (!m_encryptionKey.isEmpty()) {
                const QByteArray plain = RowEncryption::decrypt(encoded, m_encryptionKey);
                if (plain.isEmpty()) {
#if 0 // QT_DEBUG
                    qWarning() << "Pull: decryption failed for ID" << recordId;
#endif
                    ++tableResult.decryptionFailures;
                    continue;
                }
                const QJsonDocument rowDoc = QJsonDocument::fromJson(plain);
                if (rowDoc.isObject())
                    rowData = rowDoc.object();
            } else {
#if 0 // QT_DEBUG
                qWarning() << "Pull: received encrypted JSONROWDATA but no key set for ID" << recordId;
#endif
                ++tableResult.decryptionFailures;
                continue;
            }
        } else {
            rowData = jrdVal.toObject();
        }

        if (serverSmta > maxBatchSmta)
            maxBatchSmta = serverSmta;

        if (rowData.isEmpty()) {
#if 0 // QT_DEBUG
            qWarning() << "Pull: empty JSONROWDATA for ID" << recordId;
#endif
            markFailure(recordId);
            continue;
        }

        // Tracks whether this record was successfully reconciled with the local
        // database.  Only successful records advance the pull high-water mark;
        // failures hold maxSuccessSmta back so the record is retried on the next cycle.
        bool ok = false;

        if (m_dbLock) m_dbLock->lockForWrite();

        // Does this record exist locally?
        QSqlQuery existQ(m_db);
        existQ.prepare(QStringLiteral(
            "SELECT \"%1\" FROM \"%2\" WHERE \"%3\" = :id"
        ).arg(config.updatedDateColumn, config.tableName, config.idColumn));
        existQ.bindValue(QStringLiteral(":id"), recordId);

        if (!existQ.exec()) {
            if (m_dbLock) m_dbLock->unlock();
#if 0 // QT_DEBUG
            qWarning() << "Existence check failed for" << recordId
                       << existQ.lastError().text();
            qWarning().noquote() << "[SyncEngine] Failed SQL:" << debugQuery(existQ);
#endif
            markFailure(recordId);
            continue;
        }

        if (existQ.next()) {
            // Record exists locally – update only if server is newer
            const qint64 localTs = existQ.value(0).toLongLong();

            if (serverTs <= localTs) {
                if (m_dbLock) m_dbLock->unlock();
                if (localTs > serverTs) {
                    ++tableResult.conflicts;
#if 0 // QT_DEBUG
                    qDebug().noquote() << QStringLiteral("[SyncEngine] Pull '%1': skipping id=%2 (local ts=%3 > server ts=%4)")
                                              .arg(config.tableName, recordId).arg(localTs).arg(serverTs);
#endif
                }
                ok = true;  // local is already at or ahead of server — safe to advance past
            } else {
                // Build dynamic UPDATE from JSONROWDATA keys; add UPDATEDDATE and SYNCDATE explicitly
                QStringList setClauses;
                for (auto it = rowData.constBegin(); it != rowData.constEnd(); ++it)
                    setClauses << QStringLiteral("\"%1\" = :%1").arg(it.key());
                setClauses << QStringLiteral("\"%1\" = :__updateddate").arg(config.updatedDateColumn);
                setClauses << QStringLiteral("\"%1\" = :__syncdate").arg(config.syncDateColumn);

                QSqlQuery updateQ(m_db);
                updateQ.prepare(QStringLiteral("UPDATE \"%1\" SET %2 WHERE \"%3\" = :__id")
                                    .arg(config.tableName,
                                         setClauses.join(QStringLiteral(", ")),
                                         config.idColumn));

                for (auto it = rowData.constBegin(); it != rowData.constEnd(); ++it) {
                    updateQ.bindValue(QStringLiteral(":%1").arg(it.key()),
                                      SchemaInspector::jsonValueToVariant(
                                          it.value(), typeMap.value(it.key())));
                }
                updateQ.bindValue(QStringLiteral(":__updateddate"), serverTs);
                updateQ.bindValue(QStringLiteral(":__syncdate"), serverTs);
                updateQ.bindValue(QStringLiteral(":__id"), recordId);

                if (updateQ.exec()) {
                    ++pulled;
                    ok = true;
                    if (m_dbLock) m_dbLock->unlock();
                    emit progress(config.tableName, pulled, -1);
                    emit rowChanged(config.tableName, recordId);
                } else {
                    if (m_dbLock) m_dbLock->unlock();
#if 0 // QT_DEBUG
                    qWarning() << "UPDATE failed for" << recordId
                               << updateQ.lastError().text();
                    qWarning().noquote() << "[SyncEngine] Failed SQL:" << debugQuery(updateQ);
#endif
                }
            }

        } else {
            // Record does not exist locally → INSERT from JSONROWDATA
            // ID, UPDATEDDATE, and SYNCDATE are stored as top-level columns, not in JSONROWDATA
            QStringList cols, placeholders;
            for (auto it = rowData.constBegin(); it != rowData.constEnd(); ++it) {
                cols         << QStringLiteral("\"%1\"").arg(it.key());
                placeholders << QStringLiteral(":%1").arg(it.key());
            }
            cols         << QStringLiteral("\"%1\"").arg(config.idColumn);
            placeholders << QStringLiteral(":__id");
            cols         << QStringLiteral("\"%1\"").arg(config.updatedDateColumn);
            placeholders << QStringLiteral(":__updateddate");
            cols         << QStringLiteral("\"%1\"").arg(config.syncDateColumn);
            placeholders << QStringLiteral(":__syncdate");

            QSqlQuery insertQ(m_db);
            insertQ.prepare(QStringLiteral("INSERT INTO \"%1\" (%2) VALUES (%3)")
                                .arg(config.tableName,
                                     cols.join(QStringLiteral(", ")),
                                     placeholders.join(QStringLiteral(", "))));

            for (auto it = rowData.constBegin(); it != rowData.constEnd(); ++it) {
                insertQ.bindValue(QStringLiteral(":%1").arg(it.key()),
                                  SchemaInspector::jsonValueToVariant(
                                      it.value(), typeMap.value(it.key())));
            }
            insertQ.bindValue(QStringLiteral(":__id"),          recordId);
            insertQ.bindValue(QStringLiteral(":__updateddate"), serverTs);
            insertQ.bindValue(QStringLiteral(":__syncdate"),    serverTs);

            if (insertQ.exec()) {
                ++pulled;
                ok = true;
                if (m_dbLock) m_dbLock->unlock();
                emit progress(config.tableName, pulled, -1);
                emit rowChanged(config.tableName, recordId);
            } else {
                const QString dbErr = insertQ.lastError().databaseText();
                const bool isUniqueViolation =
                    dbErr.contains(QLatin1String("UNIQUE constraint failed"),
                                   Qt::CaseInsensitive);

                if (isUniqueViolation) {
                    // Another client inserted the same logical record under a different id.
                    // Rename the local id to match the server's canonical id so conflict
                    // resolution can proceed normally on the next sync cycle.
                    const QString conflictingId =
                        findConflictingLocalId(config, rowData, typeMap);

                    if (!conflictingId.isEmpty()) {
                        QSqlQuery fixQ(m_db);
                        fixQ.prepare(QStringLiteral(
                            "UPDATE \"%1\" SET \"%2\" = :newId, \"%3\" = NULL"
                            " WHERE \"%2\" = :oldId"
                        ).arg(config.tableName, config.idColumn, config.syncDateColumn));
                        fixQ.bindValue(QStringLiteral(":newId"), recordId);
                        fixQ.bindValue(QStringLiteral(":oldId"), conflictingId);
                        fixQ.exec();

                        if (m_dbLock) m_dbLock->unlock();

                        // Delete the orphaned remote record that used the old local id.
                        QUrlQuery delQuery;
                        delQuery.addQueryItem(
                            QStringLiteral("tablename"),
                            QStringLiteral("eq.%1").arg(config.tableName));
                        delQuery.addQueryItem(
                            QStringLiteral("id"),
                            QStringLiteral("eq.%1").arg(postgrestFilterValue(conflictingId)));
                        m_httpClient->deleteRow(m_postgresTableName, delQuery);
                        tableResult.bytesPushed += m_httpClient->lastBytesSent();
                        tableResult.bytesPulled += m_httpClient->lastBytesReceived();
#if 0 // QT_DEBUG
                        if (!m_httpClient->wasSuccessful()) {
                            qWarning().noquote()
                                << QStringLiteral("[SyncEngine] Could not delete orphaned"
                                                  " remote id=%1 (table '%2'): %3")
                                       .arg(conflictingId, config.tableName,
                                            m_httpClient->lastError());
                        }
                        qWarning().noquote()
                            << QStringLiteral("[SyncEngine] Unique key conflict resolved:"
                                              " local id %1 → %2 (table '%3')")
                                   .arg(conflictingId, recordId, config.tableName);
#endif
                        ok = true;  // conflict resolved — local row will sync on next push
                    } else {
                        if (m_dbLock) m_dbLock->unlock();
#if 0 // QT_DEBUG
                        qWarning() << "INSERT unique conflict but no matching local row found"
                                   << "for" << recordId << "in" << config.tableName;
#endif
                    }
                } else {
                    if (m_dbLock) m_dbLock->unlock();
#if 0 // QT_DEBUG
                    qWarning() << "INSERT failed for" << recordId
                               << insertQ.lastError().text();
                    qWarning().noquote() << "[SyncEngine] Failed SQL:" << debugQuery(insertQ);
#endif
                }
            }
        }

        if (ok) {
            ++successCount;
            // Trust the server's ORDER BY (server_modified_at.asc, id.asc).
            // Computing a "max" with C++ byte compare would disagree with
            // Postgres collation when ids mix formats (e.g. "{uuid}" vs
            // "159…"), causing the cursor to never advance past a brace-
            // wrapped value when later numeric-id rows are byte-less but
            // collation-greater.
            maxSuccessSmta = serverSmta;
            maxSuccessId   = recordId;
        } else {
            markFailure(recordId);
        }
    }

    // --- Step 3: advance the cursor to the max successful (smta, id) ---
    //
    // The cursor advances to the highest (server_modified_at, id) pair among
    // successfully processed rows, stored in maxSuccessSmta / maxSuccessId.
    // Using the successful set (not just the last row) prevents a failed row
    // at the end of the batch from blocking cursor progress.
    //
    // Two failure regimes:
    //   (a) Some records succeeded → advance the cursor to the highest
    //       successful (smta, id). Failed records are skipped (with warnings)
    //       so a single bad row never stalls progress.
    //   (b) No records succeeded but failures occurred → likely a systemic
    //       issue (transient SQL error, schema mismatch on every row, etc.).
    //       Leave the cursor unchanged so the next cycle retries the same batch.
    const bool fullBatchFailure = (successCount == 0 && !failedIds.isEmpty());
    const bool anySuccess = (successCount > 0);
    const bool shouldAdvance = !serverRows.isEmpty() && anySuccess && !fullBatchFailure;

    qint64  newCursorSmta = lastPull;
    QString newCursorId   = lastId;
    bool    cursorAdvances = false;

    if (shouldAdvance) {
        newCursorSmta  = maxSuccessSmta;
        newCursorId    = maxSuccessId;
        cursorAdvances = true;
    }

#if 0 // QT_DEBUG
    qDebug().noquote()
        << QStringLiteral("[SyncEngine] Pull '%1' summary: pulled=%2, success=%3, failed=%4, "
                          "maxSuccessSmta=%5, maxSuccessId=%6, maxBatchSmta=%7, oldCursor=(%8,'%9'), newCursor=(%10,'%11')%12")
               .arg(config.tableName).arg(pulled).arg(successCount).arg(failedIds.size())
               .arg(maxSuccessSmta).arg(maxSuccessId).arg(maxBatchSmta)
               .arg(lastPull).arg(lastId)
               .arg(newCursorSmta).arg(newCursorId)
               .arg(cursorAdvances ? QString() : QStringLiteral(" (HELD)"));
#endif

#if 0 // QT_DEBUG
    if (!failedIds.isEmpty()) {
        const QString sample = failedIds.mid(0, 10).join(QStringLiteral(", "));
        const QString more = failedIds.size() > 10
                                 ? QStringLiteral(", … (+%1 more)").arg(failedIds.size() - 10)
                                 : QString();
        if (fullBatchFailure) {
            qWarning().noquote()
                << QStringLiteral("[SyncEngine] Pull '%1': %2 record(s) failed and NONE succeeded; "
                                  "cursor held at (%3,'%4') (will retry next cycle). IDs: %5%6")
                       .arg(config.tableName).arg(failedIds.size())
                       .arg(newCursorSmta).arg(newCursorId)
                       .arg(sample, more);
        } else {
            qWarning().noquote()
                << QStringLiteral("[SyncEngine] Pull '%1': %2 record(s) skipped due to apply errors; "
                                  "cursor advanced to (%3,'%4') to keep sync progressing. "
                                  "Investigate per-record errors above. IDs: %5%6")
                       .arg(config.tableName).arg(failedIds.size())
                       .arg(newCursorSmta).arg(newCursorId)
                       .arg(sample, more);
        }
    }
#endif

    if (cursorAdvances) {
        if (m_dbLock) m_dbLock->lockForWrite();
        setLastPullCursor(config.tableName, newCursorSmta, newCursorId);
        if (m_dbLock) m_dbLock->unlock();
    }

    return pulled;
}
