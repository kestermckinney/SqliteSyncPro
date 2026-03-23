#include "syncloopworker.h"
#include "httpclient.h"
#include "syncengine.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

SyncLoopWorker::SyncLoopWorker(const QString               &dbPath,
                                const QString               &serverUrl,
                                const QString               &authToken,
                                const QString               &supabaseKey,
                                const QString               &userId,
                                const QString               &postgresTableName,
                                const QList<SyncTableConfig> &tables,
                                QReadWriteLock              *dbLock,
                                const QByteArray            &encryptionKey,
                                int                          syncIntervalMs,
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
    , m_syncIntervalMs(syncIntervalMs)
{
}

void SyncLoopWorker::run()
{
    const QString connName = QStringLiteral("SqliteSyncPro_loop_%1")
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
        result.errorMessage = QStringLiteral("SyncLoopWorker: cannot open database: %1")
                                  .arg(db.lastError().text());
        cleanupDb();
        emit syncCompleted(result);
        emit finished();
        return;
    }

    {
        // Scope engine (and http) so they are destroyed — releasing their QSqlDatabase
        // copies — before cleanupDb() calls removeDatabase().
        HttpClient http;
        http.setBaseUrl(m_serverUrl);
        http.setAuthToken(m_authToken);
        http.setApiKey(m_supabaseKey);

        SyncEngine engine;
        engine.setDatabase(db);
        engine.setHttpClient(&http);
        engine.setUserId(m_userId);
        engine.setPostgresTableName(m_postgresTableName);
        engine.setDatabaseLock(m_dbLock);
        engine.setEncryptionKey(m_encryptionKey);

        connect(&engine, &SyncEngine::progress,    this, &SyncLoopWorker::progress);
        connect(&engine, &SyncEngine::rowChanged,  this, &SyncLoopWorker::rowChanged);

        while (true) {
            {
                QMutexLocker locker(&m_stopMutex);
                if (m_stopRequested)
                    break;
            }

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
            emit syncCompleted(combined);

            // Stop the loop on decryption failures — the phrase is wrong and
            // continuing would show the same warning dialog on every interval.
            if (combined.totalDecryptionFailures() > 0)
                break;

            // Sleep for the interval, waking early if stop is requested.
            QMutexLocker locker(&m_stopMutex);
            if (!m_stopRequested)
                m_stopCondition.wait(&m_stopMutex,
                                     static_cast<unsigned long>(m_syncIntervalMs));
            if (m_stopRequested)
                break;
        }
    } // engine and http destroyed here; db is the only remaining reference

    cleanupDb();
    emit finished();
}

void SyncLoopWorker::requestStop()
{
    QMutexLocker locker(&m_stopMutex);
    m_stopRequested = true;
    m_stopCondition.wakeAll();
}
