// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>

/**
 * TeardownWorker drops all database objects created by the setup wizard.
 * Each step emits stepCompleted(); all steps run even if some fail so that
 * as many objects as possible are removed.  finished() is emitted at the end
 * with overall success/failure.
 */
class TeardownWorker : public QObject
{
    Q_OBJECT

public:
    explicit TeardownWorker(QObject *parent = nullptr);

public slots:
    void runTeardown(const QString &host,
                     int            port,
                     const QString &dbName,
                     const QString &superuser,
                     const QString &superPass,
                     bool           supabaseMode);

signals:
    void stepCompleted(int index, bool success,
                       const QString &stepName,
                       const QString &detail);

    void finished(bool success, const QString &errorMessage);

private:
    bool execStep(int &counter, const QString &name, const QString &sql);

    QSqlDatabase m_db;
    QString      m_connName;
};
