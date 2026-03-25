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
        "  last_pull_time INTEGER NOT NULL DEFAULT 0"
        ")"
    ));
}

qint64 SyncEngine::getLastPullTime(const QString &tableName)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT last_pull_time FROM _sync_meta WHERE table_name = :name"
    ));
    q.bindValue(QStringLiteral(":name"), tableName);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();
    return 0;
}

void SyncEngine::setLastPullTime(const QString &tableName, qint64 utcMs)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO _sync_meta (table_name, last_pull_time) VALUES (:name, :ts)"
    ));
    q.bindValue(QStringLiteral(":name"), tableName);
    q.bindValue(QStringLiteral(":ts"),   utcMs);
    q.exec();
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

    const int pushed = pushLocalChanges(config, tableResult);
    if (pushed < 0) {
        tableResult.success = false;
        result.success      = false;
        result.tableResults.append(tableResult);
        return result;
    }
    tableResult.pushed = pushed;

    // If any records were pushed this round, peer devices may subsequently push
    // records whose UPDATEDDATE falls before our current last_pull_time (because
    // UPDATEDDATE reflects when the record was locally mutated, not when it was
    // pushed to the server).  Reset the high-water mark to 0 so the next pull
    // round re-scans from the beginning and picks up those early-timestamped
    // records.  The extra server scan only happens while pushing is still active;
    // once pushed == 0 the mark is left in place and normal incremental pulls resume.
    if (pushed > 0) {
        if (m_dbLock) m_dbLock->lockForWrite();
        setLastPullTime(config.tableName, 0);
        if (m_dbLock) m_dbLock->unlock();
    }

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
// Fine-grained locking strategy:
//   1. Acquire write lock → SELECT all dirty records into a buffer → release lock
//   2. For each buffered record: do all HTTP calls with NO lock held
//   3. After each successful upload: acquire write lock briefly → stamp SYNCDATE → release
// ---------------------------------------------------------------------------

int SyncEngine::pushLocalChanges(const SyncTableConfig &config, TableSyncResult &tableResult)
{
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

        QSqlQuery selectQ(m_db);
        if (!selectQ.exec(selectSql)) {
            if (m_dbLock) m_dbLock->unlock();
            tableResult.errorMessage = QStringLiteral("Failed to query local records: %1")
                                           .arg(selectQ.lastError().text());
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

        if (m_dbLock) m_dbLock->unlock();
    }
    // Write lock is now released; HTTP calls follow with no lock held.

    int pushed = 0;

    // --- Step 2: process each buffered record – HTTP only, no lock held ---
    for (const PendingRecord &pr : pending) {
        const QString    &recordId = pr.id;
        const qint64      localTs  = pr.ts;
        const QJsonObject &rowJson = pr.rowJson;

        // Check whether this record already exists on the server
        QUrlQuery checkQuery;
        checkQuery.addQueryItem(QStringLiteral("tablename"),
                                QStringLiteral("eq.%1").arg(config.tableName));
        checkQuery.addQueryItem(QStringLiteral("id"),
                                QStringLiteral("eq.%1").arg(recordId));
        checkQuery.addQueryItem(QStringLiteral("select"),
                                QStringLiteral("id,updateddate"));

        const QByteArray checkResp = m_httpClient->get(m_postgresTableName, checkQuery);
        if (!m_httpClient->wasSuccessful()) {
            tableResult.errorMessage =
                QStringLiteral("Server check failed for record %1: %2")
                    .arg(recordId, m_httpClient->lastError());
            if (m_httpClient->lastStatusCode() == 0) {
                tableResult.networkError = true;
                qWarning().noquote() << QStringLiteral("[SyncEngine] Network error pushing record %1 ('%2'): server unreachable (status 0)")
                                            .arg(recordId, config.tableName);
            } else {
                qWarning().noquote() << QStringLiteral("[SyncEngine] Push check failed for record %1 ('%2'): HTTP %3 — %4")
                                            .arg(recordId, config.tableName)
                                            .arg(m_httpClient->lastStatusCode())
                                            .arg(m_httpClient->lastError());
            }
            return -1;
        }

        const QJsonArray checkArr = QJsonDocument::fromJson(checkResp).array();

        bool uploadOk = false;

        // Serialise rowJson; encrypt if a key is set.
        QJsonValue jsonRowDataValue;
        if (!m_encryptionKey.isEmpty()) {
            const QByteArray plain = QJsonDocument(rowJson).toJson(QJsonDocument::Compact);
            const QString enc = RowEncryption::encrypt(plain, m_encryptionKey);
            if (enc.isEmpty()) {
                qWarning() << "Encryption failed for record" << recordId << "– skipping";
                continue;
            }
            jsonRowDataValue = enc;
        } else {
            jsonRowDataValue = rowJson;
        }

        if (checkArr.isEmpty()) {
            // Record does not exist on server → POST
            QJsonObject payload;
            if (!m_userId.isEmpty())
                payload[QStringLiteral("userid")]      = m_userId;
            payload[QStringLiteral("tablename")]   = config.tableName;
            payload[QStringLiteral("id")]          = recordId;
            payload[QStringLiteral("updateddate")] = localTs;
            payload[QStringLiteral("jsonrowdata")] = jsonRowDataValue;

            m_httpClient->post(m_postgresTableName,
                               QJsonDocument(payload).toJson(QJsonDocument::Compact),
                               {QStringLiteral("return=minimal")});
            uploadOk = m_httpClient->wasSuccessful();
            if (!uploadOk)
                qWarning() << "POST failed for" << recordId << m_httpClient->lastError();

        } else {
            const qint64 serverTs = checkArr.first().toObject()
                                        .value(QStringLiteral("updateddate"))
                                        .toVariant().toLongLong();

            if (localTs > serverTs) {
                // Local is newer → PATCH
                QUrlQuery patchQuery;
                patchQuery.addQueryItem(QStringLiteral("tablename"),
                                        QStringLiteral("eq.%1").arg(config.tableName));
                patchQuery.addQueryItem(QStringLiteral("id"),
                                        QStringLiteral("eq.%1").arg(recordId));

                QJsonObject payload;
                payload[QStringLiteral("updateddate")] = localTs;
                payload[QStringLiteral("jsonrowdata")] = jsonRowDataValue;

                m_httpClient->patch(m_postgresTableName, patchQuery,
                                    QJsonDocument(payload).toJson(QJsonDocument::Compact));
                uploadOk = m_httpClient->wasSuccessful();
                if (!uploadOk)
                    qWarning() << "PATCH failed for" << recordId << m_httpClient->lastError();

            } else if (serverTs > localTs) {
                // Server is newer → conflict; server wins; resolved in pull phase
                ++tableResult.conflicts;
                continue;
            } else {
                // Equal timestamps → already in sync; just stamp SYNCDATE
                uploadOk = true;
            }
        }

        // --- Step 3: stamp SYNCDATE under a brief write lock ---
        if (uploadOk) {
            if (m_dbLock) m_dbLock->lockForWrite();

            QSqlQuery stampQ(m_db);
            stampQ.prepare(QStringLiteral(
                "UPDATE \"%1\" SET \"%2\" = \"%3\" WHERE \"%4\" = :id"
            ).arg(config.tableName, config.syncDateColumn,
                  config.updatedDateColumn, config.idColumn));
            stampQ.bindValue(QStringLiteral(":id"), recordId);
            if (stampQ.exec()) {
                ++pushed;
                if (m_dbLock) m_dbLock->unlock();
                emit progress(config.tableName, pushed, -1);
            } else {
                if (m_dbLock) m_dbLock->unlock();
                qWarning() << "Failed to stamp SYNCDATE for" << recordId
                           << stampQ.lastError().text();
            }
        }
    }

    return pushed;
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

    const qint64 lastPull = getLastPullTime(config.tableName);

    // --- Step 1: HTTP GET with no lock held ---
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("tablename"),
                       QStringLiteral("eq.%1").arg(config.tableName));
    // Use gte (>=) so records whose updateddate equals last_pull_time are not
    // permanently missed.  Records already present locally are harmlessly
    // skipped by the serverTs <= localTs check below.
    query.addQueryItem(QStringLiteral("updateddate"),
                       QStringLiteral("gte.%1").arg(lastPull));
    query.addQueryItem(QStringLiteral("order"),
                       QStringLiteral("updateddate.asc"));
    query.addQueryItem(QStringLiteral("limit"), QString::number(config.batchSize));

    const QByteArray response = m_httpClient->get(m_postgresTableName, query);
    if (!m_httpClient->wasSuccessful()) {
        tableResult.errorMessage = QStringLiteral("Pull request failed: %1")
                                       .arg(m_httpClient->lastError());
        if (m_httpClient->lastStatusCode() == 0) {
            tableResult.networkError = true;
            qWarning().noquote() << QStringLiteral("[SyncEngine] Network error pulling '%1': server unreachable (status 0)")
                                        .arg(config.tableName);
        } else {
            qWarning().noquote() << QStringLiteral("[SyncEngine] Pull failed for '%1': HTTP %2 — %3")
                                        .arg(config.tableName)
                                        .arg(m_httpClient->lastStatusCode())
                                        .arg(m_httpClient->lastError());
        }
        return -1;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isArray()) {
        tableResult.errorMessage = QStringLiteral("Unexpected response format from server during pull");
        return -1;
    }

    const QJsonArray serverRows = doc.array();
    int    pulled      = 0;
    qint64 maxPullTime = lastPull;

    // --- Step 2: upsert each record under a per-record write lock ---
    for (const QJsonValue &val : serverRows) {
        if (!val.isObject())
            continue;

        const QJsonObject serverRow = val.toObject();
        const qint64      serverTs  = serverRow.value(QStringLiteral("updateddate"))
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
                    qWarning() << "Pull: decryption failed for ID" << recordId;
                    ++tableResult.decryptionFailures;
                    continue;
                }
                const QJsonDocument rowDoc = QJsonDocument::fromJson(plain);
                if (rowDoc.isObject())
                    rowData = rowDoc.object();
            } else {
                qWarning() << "Pull: received encrypted JSONROWDATA but no key set for ID" << recordId;
                ++tableResult.decryptionFailures;
                continue;
            }
        } else {
            rowData = jrdVal.toObject();
        }

        if (rowData.isEmpty()) {
            qWarning() << "Pull: empty JSONROWDATA for ID" << recordId;
            continue;
        }

        if (serverTs > maxPullTime)
            maxPullTime = serverTs;

        if (m_dbLock) m_dbLock->lockForWrite();

        // Does this record exist locally?
        QSqlQuery existQ(m_db);
        existQ.prepare(QStringLiteral(
            "SELECT \"%1\" FROM \"%2\" WHERE \"%3\" = :id"
        ).arg(config.updatedDateColumn, config.tableName, config.idColumn));
        existQ.bindValue(QStringLiteral(":id"), recordId);

        if (!existQ.exec()) {
            if (m_dbLock) m_dbLock->unlock();
            qWarning() << "Existence check failed for" << recordId
                       << existQ.lastError().text();
            continue;
        }

        if (existQ.next()) {
            // Record exists locally – update only if server is newer
            const qint64 localTs = existQ.value(0).toLongLong();

            if (serverTs <= localTs) {
                if (m_dbLock) m_dbLock->unlock();
                if (localTs > serverTs)
                    ++tableResult.conflicts;
                continue;
            }

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
                if (m_dbLock) m_dbLock->unlock();
                emit progress(config.tableName, pulled, -1);
                emit rowChanged(config.tableName, recordId);
            } else {
                if (m_dbLock) m_dbLock->unlock();
                qWarning() << "UPDATE failed for" << recordId
                           << updateQ.lastError().text();
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
                if (m_dbLock) m_dbLock->unlock();
                emit progress(config.tableName, pulled, -1);
                emit rowChanged(config.tableName, recordId);
            } else {
                if (m_dbLock) m_dbLock->unlock();
                qWarning() << "INSERT failed for" << recordId
                           << insertQ.lastError().text();
            }
        }
    }

    // --- Step 3: persist the high-water mark under a brief write lock ---
    if (maxPullTime > lastPull) {
        if (m_dbLock) m_dbLock->lockForWrite();
        setLastPullTime(config.tableName, maxPullTime);
        if (m_dbLock) m_dbLock->unlock();
    }

    return pulled;
}
