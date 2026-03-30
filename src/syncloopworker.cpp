#include "syncloopworker.h"
#include "authmanager.h"
#include "httpclient.h"
#include "syncengine.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrlQuery>
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
                                int                          syncHostType,
                                const QString               &email,
                                const QString               &password,
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
    , m_syncHostType(syncHostType)
    , m_email(email)
    , m_password(password)
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

#ifdef QT_DEBUG
        qDebug().noquote() << QStringLiteral("[SyncLoopWorker] Starting sync loop for %1 table(s): %2")
                                  .arg(m_tables.size())
                                  .arg([this]() {
                                      QStringList names;
                                      for (const auto &t : m_tables)
                                          names << t.tableName;
                                      return names.join(QStringLiteral(", "));
                                  }());
#endif

        while (true) {
            {
                QMutexLocker locker(&m_stopMutex);
                if (m_stopRequested)
                    break;
            }

#ifdef QT_DEBUG
            qDebug().noquote() << QStringLiteral("[SyncLoopWorker] Sync cycle starting");
#endif

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

#ifdef QT_DEBUG
            qDebug().noquote() << QStringLiteral("[SyncLoopWorker] Sync cycle complete: pulled=%1, pushed=%2, conflicts=%3, success=%4%5")
                                      .arg(combined.totalPulled())
                                      .arg(combined.totalPushed())
                                      .arg(combined.totalConflicts())
                                      .arg(combined.success ? QStringLiteral("true") : QStringLiteral("false"))
                                      .arg(combined.success ? QString() : QStringLiteral(", error: ") + combined.errorMessage);
#endif

            // Query pending counts so we can log how much work remains.
            // Local dirty count (syncdate IS NULL) → still needs push.
            // Server count filtered by updateddate >= lastPullTime → still needs pull
            // (same filter as the actual pull, so it counts exactly what remains).
            {
                qint64 pendingPush = 0;
                QList<qint64> lastPullTimes;
                lastPullTimes.reserve(m_tables.size());

                {
                    QReadLocker lock(m_dbLock);
                    for (const auto &cfg : m_tables) {
                        QSqlQuery q(db);
                        q.exec(QStringLiteral("SELECT COUNT(*) FROM \"%1\" WHERE syncdate IS NULL")
                                   .arg(cfg.tableName));
                        if (q.next())
                            pendingPush += q.value(0).toLongLong();

                        qint64 lastPull = 0;
                        q.prepare(QStringLiteral(
                            "SELECT last_pull_time FROM _sync_meta WHERE table_name = :name"));
                        q.bindValue(QStringLiteral(":name"), cfg.tableName);
                        q.exec();
                        if (q.next())
                            lastPull = q.value(0).toLongLong();
                        lastPullTimes << lastPull;
                    }
                }

                qint64 pendingPull = 0;
                bool   pullUnknown = false;

                if (combined.hasNetworkError()) {
                    pullUnknown = true;
                } else if (combined.totalPulled() > 0) {
                    // Records came down this cycle — query the server to see how many remain.
                    for (int i = 0; i < m_tables.size(); ++i) {
                        QUrlQuery query;
                        query.addQueryItem(QStringLiteral("tablename"),
                                           QStringLiteral("eq.%1").arg(m_tables[i].tableName));
                        query.addQueryItem(QStringLiteral("updateddate"),
                                           QStringLiteral("gt.%1").arg(lastPullTimes[i]));
                        const int serverCount = http.countRows(m_postgresTableName, query);
                        if (serverCount < 0)
                            pullUnknown = true;
                        else
                            pendingPull += serverCount;
                    }
                }
                // else: pulled 0 with no network error — lastPullTime has advanced past all
                // available server records (or everything was conflict-skipped); pendingPull = 0.

#ifdef QT_DEBUG
                qDebug().noquote()
                    << QStringLiteral("[SyncLoopWorker] Remaining: push=%1, pull=%2")
                           .arg(pendingPush)
                           .arg(pullUnknown ? QStringLiteral("unknown (server unreachable)")
                                           : QString::number(pendingPull));
#endif
            }

            emit syncCompleted(combined);

            // Stop the loop on decryption failures — the phrase is wrong and
            // continuing would show the same warning dialog on every interval.
            if (combined.totalDecryptionFailures() > 0)
                break;

            // On 401, try to reauthenticate with the stored credentials.
            // If reauth succeeds, update the token and retry immediately.
            // If reauth fails, emit authenticationRequired and stop.
            if (combined.hasAuthError()) {
#ifdef QT_DEBUG
                qWarning().noquote() << QStringLiteral("[SyncLoopWorker] JWT expired; attempting reauthentication");
#endif

                QString authEndpoint;
                HttpClient authHttp;
                authHttp.setApiKey(m_supabaseKey);

                if (m_syncHostType == 1) {
                    // Supabase: m_serverUrl is the /rest/v1 URL; derive the base URL for auth
                    QString baseUrl = m_serverUrl;
                    if (baseUrl.endsWith(QStringLiteral("/rest/v1")))
                        baseUrl.chop(8);
                    else if (baseUrl.endsWith(QStringLiteral("/rest/v1/")))
                        baseUrl.chop(9);
                    authEndpoint = baseUrl + QStringLiteral("/auth/v1/token?grant_type=password");
                } else {
                    // Self-hosted: relative endpoint on the same server
                    authHttp.setBaseUrl(m_serverUrl);
                    authEndpoint = QStringLiteral("rpc/rpc_login");
                }

                AuthManager auth;
                if (auth.login(&authHttp, authEndpoint, m_email, m_password)) {
                    m_authToken = auth.token();
                    http.setAuthToken(m_authToken);
#ifdef QT_DEBUG
                    qDebug().noquote() << QStringLiteral("[SyncLoopWorker] Reauthentication succeeded; resuming sync");
#endif
                    continue;  // retry cycle immediately without sleeping
                } else {
#ifdef QT_DEBUG
                    qWarning().noquote() << QStringLiteral("[SyncLoopWorker] Reauthentication failed; stopping sync loop");
#endif
                    emit authenticationRequired();
                    break;
                }
            }

            // Back off when the server is unreachable so we don't flood the log
            // with repeated failures.  Use 5× the normal interval, capped at 5 minutes.
            int waitMs = m_syncIntervalMs;
            if (combined.hasNetworkError()) {
                waitMs = qMin(m_syncIntervalMs * 5, 300000);
#ifdef QT_DEBUG
                qWarning().noquote()
                    << QStringLiteral("[SyncLoopWorker] Network unreachable; backing off %1 ms before retry")
                           .arg(waitMs);
#endif
            }

            // Sleep for the interval, waking early if stop is requested.
            QMutexLocker locker(&m_stopMutex);
            if (!m_stopRequested)
                m_stopCondition.wait(&m_stopMutex, static_cast<unsigned long>(waitMs));
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
