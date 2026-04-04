// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QString>
#include <QWaitCondition>

#include "syncconfig.h"
#include "syncresult.h"

/**
 * SyncLoopWorker – runs a continuous sync loop on whatever thread it is moved to.
 *
 * On each iteration the worker syncs all configured tables then sleeps for
 * syncIntervalMs.  Call requestStop() to wake the sleep early and exit the
 * loop cleanly; the finished() signal is emitted when the loop exits.
 *
 * Like SyncWorker, the loop worker owns its own QSqlDatabase connection and
 * HttpClient so it is completely independent of the calling thread's resources.
 */
class SyncLoopWorker : public QObject
{
    Q_OBJECT

public:
    explicit SyncLoopWorker(const QString               &dbPath,
                            const QString               &serverUrl,
                            const QString               &authToken,
                            const QString               &supabaseKey,
                            const QString               &userId,
                            const QString               &postgresTableName,
                            const QList<SyncTableConfig> &tables,
                            QReadWriteLock              *dbLock,
                            const QByteArray            &encryptionKey = {},
                            int                          syncIntervalMs = 5000,
                            int                          syncHostType   = 0,
                            const QString               &email         = {},
                            const QString               &password      = {},
                            QObject                     *parent        = nullptr);

public slots:
    /** Start the sync loop. Connect this to QThread::started. */
    void run();

    /** Signal the loop to stop and wake the sleep so it exits promptly. */
    void requestStop();

    /**
     * Wake the backoff sleep early so the next sync cycle starts immediately.
     * Unlike requestStop(), this does not set the stop flag — the loop keeps running.
     * Call this when network connectivity is restored after a failure.
     */
    void retryNow();

signals:
    void progress(const QString &tableName, int processed, int total);
    void rowChanged(const QString &tableName, const QString &id);
    void syncCompleted(SyncResult result);
    void finished();

    /**
     * Emitted when the server returns HTTP 401 and reauthentication with
     * the stored credentials also fails.  The sync loop has stopped.
     */
    void authenticationRequired();

private:
    QString                m_dbPath;
    QString                m_serverUrl;
    QString                m_authToken;
    QString                m_supabaseKey;
    QString                m_userId;
    QString                m_postgresTableName;
    QList<SyncTableConfig> m_tables;
    QReadWriteLock        *m_dbLock;
    QByteArray             m_encryptionKey;
    int                    m_syncIntervalMs;
    int                    m_syncHostType;
    QString                m_email;
    QString                m_password;

    QMutex         m_stopMutex;
    QWaitCondition m_stopCondition;
    bool           m_stopRequested = false;
};
