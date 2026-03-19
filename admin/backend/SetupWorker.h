#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>

#include "ServerMode.h"

/**
 * SetupWorker runs all Postgres DDL steps on a dedicated QThread so the UI
 * remains responsive during the (potentially slow) CREATE EXTENSION / bcrypt
 * function installations.
 *
 * Lifecycle
 * ---------
 * 1. AdminController moves this object to a QThread and starts it.
 * 2. AdminController invokes runSetup() via a queued signal/slot connection.
 * 3. Each SQL step emits stepCompleted(); the whole sequence emits finished().
 * 4. On failure, outstanding work is rolled back and finished(false, …) fires.
 *
 * The connection opened inside runSetup() is closed and removed before
 * finished() is emitted, so there are no dangling handles.
 */
class SetupWorker : public QObject
{
    Q_OBJECT

public:
    explicit SetupWorker(QObject *parent = nullptr);

public slots:
    void runSetup(const QString &host,
                  int            port,
                  const QString &dbName,
                  const QString &superuser,
                  const QString &superPass,
                  const QString &authenticatorPassword,
                  const QString &jwtSecret,
                  bool           supabaseMode);

    void cancel();

signals:
    /**
     * Emitted after each SQL step completes.
     * @param index     0-based step index
     * @param success   false if the step failed (setup will be aborted)
     * @param stepName  Human-readable label shown in the UI log
     * @param detail    Extra info (empty on success, error text on failure)
     */
    void stepCompleted(int index, bool success,
                       const QString &stepName,
                       const QString &detail);

    void finished(bool success, const QString &errorMessage);

private:
    bool execStep(int index, const QString &name, const QString &sql);

    /**
     * Like execStep but treats failures as warnings rather than fatal errors.
     * Used in hosted-service mode for role/grant DDL that may be restricted.
     * On failure, emits stepCompleted with ok=true and a "(skipped)" suffix
     * so the UI shows a warning and setup continues.
     */
    bool execSoftStep(int index, const QString &name, const QString &sql);

    QSqlDatabase m_db;
    QString      m_connName;
    bool         m_cancelled     = false;
    bool         m_inTransaction = false;
    bool         m_supabaseMode  = false;
};
