// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QByteArray>
#include <QObject>
#include <QReadWriteLock>
#include <QString>
#include <QList>

#include "syncconfig.h"
#include "syncresult.h"

/**
 * SyncWorker – runs a full sync session on whatever thread it is executed in.
 *
 * Owns its own QSqlDatabase connection and HttpClient, so it is completely
 * independent of any other thread's resources.  Intended to be moved to a
 * QThread via moveToThread() and triggered via a queued slot call.
 *
 * The dbLock must refer to the same QReadWriteLock that the calling application
 * uses for its own database access.  SyncWorker acquires a write lock for each
 * table batch and releases it between tables (never held during network I/O).
 */
class SyncWorker : public QObject
{
    Q_OBJECT

public:
    explicit SyncWorker(const QString               &dbPath,
                        const QString               &serverUrl,
                        const QString               &authToken,
                        const QString               &supabaseKey,
                        const QString               &userId,
                        const QString               &postgresTableName,
                        const QList<SyncTableConfig> &tables,
                        QReadWriteLock              *dbLock,
                        const QByteArray            &encryptionKey = {},
                        QObject                     *parent        = nullptr);

public slots:
    void run();

signals:
    void progress(const QString &tableName, int processed, int total);
    void finished(SyncResult result);

private:
    QString                m_dbPath;
    QString                m_serverUrl;
    QString                m_authToken;
    QString                m_supabaseKey;  // anon key; empty for self-hosted
    QString                m_userId;
    QString                m_postgresTableName;
    QList<SyncTableConfig> m_tables;
    QReadWriteLock        *m_dbLock;
    QByteArray             m_encryptionKey;
};
