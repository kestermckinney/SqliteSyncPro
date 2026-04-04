// Copyright (C) 2026 Paul McKinney
#include "syncworker.h"
#include "httpclient.h"
#include "syncengine.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>
#include <QUuid>
#include <QDebug>

SyncWorker::SyncWorker(const QString               &dbPath,
                        const QString               &serverUrl,
                        const QString               &authToken,
                        const QString               &supabaseKey,
                        const QString               &userId,
                        const QString               &postgresTableName,
                        const QList<SyncTableConfig> &tables,
                        QReadWriteLock              *dbLock,
                        const QByteArray            &encryptionKey,
                        QObject                     *parent)
    : QObject(parent)
    , m_dbPath(dbPath)
    , m_serverUrl(serverUrl)
    , m_authToken(authToken)
    , m_supabaseKey(supabaseKey)
    , m_userId(userId)
    , m_postgresTableName(postgresTableName)
    , m_tables(tables)
    , m_dbLock(dbLock)
    , m_encryptionKey(encryptionKey)
{
}

void SyncWorker::run()
{
    const QString connName = QStringLiteral("SqliteSyncPro_worker_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(m_dbPath);

    auto cleanupDb = [&db, &connName]() {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    };

    if (!db.open()) {
        SyncResult result;
        result.success      = false;
        result.errorMessage = QStringLiteral("Worker: cannot open database: %1")
                                  .arg(db.lastError().text());
        cleanupDb();
        emit finished(result);
        return;
    }

    // HttpClient must be created in this thread (QNetworkAccessManager thread affinity).
    HttpClient http;
    http.setBaseUrl(m_serverUrl);
    http.setAuthToken(m_authToken);
    http.setApiKey(m_supabaseKey); // empty for self-hosted; sets apikey header for Supabase

    SyncEngine engine;
    engine.setDatabase(db);
    engine.setHttpClient(&http);
    engine.setUserId(m_userId);
    engine.setPostgresTableName(m_postgresTableName);
    engine.setDatabaseLock(m_dbLock);
    engine.setEncryptionKey(m_encryptionKey);

    connect(&engine, &SyncEngine::progress, this, &SyncWorker::progress);

    SyncResult combined;

    for (const auto &cfg : m_tables) {
        const SyncResult tableSync = engine.synchronizeTable(cfg);

        for (const auto &tr : tableSync.tableResults)
            combined.tableResults.append(tr);
        if (!tableSync.success) {
            combined.success      = false;
            combined.errorMessage = tableSync.errorMessage;
        }
    }

    cleanupDb();
    emit finished(combined);
}
