// Copyright (C) 2026 Paul McKinney
#include "TeardownWorker.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

TeardownWorker::TeardownWorker(QObject *parent)
    : QObject(parent)
{
}

bool TeardownWorker::execStep(int &counter, const QString &name, const QString &sql)
{
    QSqlQuery q(m_db);
    const bool ok = q.exec(sql);
    const QString detail = ok ? QString() : q.lastError().text();
    if (!ok)
#ifdef QT_DEBUG
        qWarning().noquote() << "[TeardownWorker] FAILED:" << name << "-" << detail;
#endif
    emit stepCompleted(counter++, ok, name, detail);
    return ok;
}

void TeardownWorker::runTeardown(const QString &host,
                                  int            port,
                                  const QString &dbName,
                                  const QString &superuser,
                                  const QString &superPass,
                                  bool           supabaseMode)
{
    m_connName = QStringLiteral("teardown_%1")
                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(superuser);
    m_db.setPassword(superPass);

    if (!m_db.open()) {
        const QString err = m_db.lastError().text();
#ifdef QT_DEBUG
        qWarning() << "[TeardownWorker] Cannot open database:" << err;
#endif
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connName);
        emit finished(false, QStringLiteral("Cannot connect to database: %1").arg(err));
        return;
    }

    int s = 0;
    bool anyFailed = false;

    auto step = [&](const QString &name, const QString &sql) {
        if (!execStep(s, name, sql))
            anyFailed = true;
    };

    // ── Functions ─────────────────────────────────────────────────────────────
    step(QStringLiteral("Drop function: rpc_login"),
         QStringLiteral("DROP FUNCTION IF EXISTS rpc_login(TEXT, TEXT)"));

    step(QStringLiteral("Drop function: _verify"),
         QStringLiteral("DROP FUNCTION IF EXISTS _verify(TEXT, TEXT, TEXT)"));

    step(QStringLiteral("Drop function: _sign_jwt"),
         QStringLiteral("DROP FUNCTION IF EXISTS _sign_jwt(json, TEXT)"));

    step(QStringLiteral("Drop function: _algorithm_sign"),
         QStringLiteral("DROP FUNCTION IF EXISTS _algorithm_sign(TEXT, TEXT, TEXT)"));

    step(QStringLiteral("Drop function: _try_cast_double"),
         QStringLiteral("DROP FUNCTION IF EXISTS _try_cast_double(TEXT)"));

    step(QStringLiteral("Drop function: _url_encode"),
         QStringLiteral("DROP FUNCTION IF EXISTS _url_encode(TEXT)"));

    // ── Tables ────────────────────────────────────────────────────────────────
    // CASCADE drops all dependent objects (RLS policies, indexes, triggers).
    step(QStringLiteral("Drop table: sync_data"),
         QStringLiteral("DROP TABLE IF EXISTS sync_data CASCADE"));

    step(QStringLiteral("Drop table: auth_users"),
         QStringLiteral("DROP TABLE IF EXISTS auth_users CASCADE"));

    // ── Roles (self-hosted only) ──────────────────────────────────────────────
    // Supabase manages its own roles; we must not touch them.
    //
    // NOTE: REASSIGN OWNED BY and DROP OWNED BY are utility statements that
    // PostgreSQL does NOT allow inside a DO $$ / PL/pgSQL block.  We check
    // existence at the application level and issue each command as a direct
    // SQL statement so they actually execute.
    if (!supabaseMode) {
        for (const QString &role : {QStringLiteral("anon"),
                                     QStringLiteral("app_user"),
                                     QStringLiteral("authenticator"),
                                     QStringLiteral("postgrest_db")}) {
            // Check existence
            QSqlQuery chk(m_db);
            chk.prepare(QStringLiteral(
                "SELECT 1 FROM pg_roles WHERE rolname = :r"));
            chk.bindValue(QStringLiteral(":r"), role);
            chk.exec();

            if (!chk.next()) {
                // Role not found — emit a success/skipped step and continue.
                emit stepCompleted(s++, true,
                                   QStringLiteral("Drop role: %1 (not found, skipped)").arg(role),
                                   QString());
                continue;
            }

            // Reassign any objects the role owns to the connected superuser,
            // revoke all its privileges, then drop it.
            QSqlQuery q(m_db);
            bool ok = true;
            QString err;

            if (!q.exec(QStringLiteral("REASSIGN OWNED BY %1 TO current_user").arg(role))) {
                ok = false; err = q.lastError().text();
            }
            if (ok && !q.exec(QStringLiteral("DROP OWNED BY %1").arg(role))) {
                ok = false; err = q.lastError().text();
            }
            if (ok && !q.exec(QStringLiteral("DROP ROLE IF EXISTS %1").arg(role))) {
                ok = false; err = q.lastError().text();
            }

            if (!ok) {
#ifdef QT_DEBUG
                qWarning().noquote() << "[TeardownWorker] FAILED: Drop role:"
                                     << role << "-" << err;
#endif
                anyFailed = true;
            }
            emit stepCompleted(s++, ok,
                               QStringLiteral("Drop role: %1").arg(role), err);
        }
    }

    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connName);

#ifdef QT_DEBUG
    qDebug() << "[TeardownWorker] finished anyFailed=" << anyFailed;
#endif
    emit finished(!anyFailed,
                  anyFailed ? QStringLiteral("One or more steps failed — see log for details.")
                            : QString());
}
