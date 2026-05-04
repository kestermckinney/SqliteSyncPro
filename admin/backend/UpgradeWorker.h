// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>

/**
 * UpgradeWorker brings an older Project Notes Remote Host database forward
 * to the schema required by Project Notes 5.0.1.
 *
 * 5.0.1 introduces a server-assigned `server_modified_at` column on
 * sync_data so the pull cursor is monotonic across all clients regardless
 * of clock skew or backdated `updateddate` values.
 *
 * All steps run inside a single transaction so a failure leaves the
 * database in its original state.
 */
class UpgradeWorker : public QObject
{
    Q_OBJECT

public:
    explicit UpgradeWorker(QObject *parent = nullptr);

public slots:
    void runUpgrade(const QString &host,
                    int            port,
                    const QString &dbName,
                    const QString &superuser,
                    const QString &superPass);

signals:
    void stepCompleted(int index, bool success,
                       const QString &stepName,
                       const QString &detail);

    void finished(bool success, const QString &errorMessage);

private:
    bool execStep(int index, const QString &name, const QString &sql);

    QSqlDatabase m_db;
    QString      m_connName;
    bool         m_inTransaction = false;
};
