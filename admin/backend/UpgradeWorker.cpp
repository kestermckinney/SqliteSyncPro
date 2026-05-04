// Copyright (C) 2026 Paul McKinney
#include "UpgradeWorker.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

UpgradeWorker::UpgradeWorker(QObject *parent)
    : QObject(parent)
{
}

bool UpgradeWorker::execStep(int index, const QString &name, const QString &sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        const QString err = q.lastError().text();
        emit stepCompleted(index, false, name, err);
        return false;
    }
    emit stepCompleted(index, true, name, QString());
    return true;
}

void UpgradeWorker::runUpgrade(const QString &host,
                                int            port,
                                const QString &dbName,
                                const QString &superuser,
                                const QString &superPass)
{
    m_connName = QStringLiteral("SqlSyncAdmin_upgrade_%1")
                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(superuser);
    m_db.setPassword(superPass);

    auto cleanup = [this]() {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connName);
    };

    if (!m_db.open()) {
        emit finished(false, QStringLiteral("Cannot connect to PostgreSQL: %1")
                                 .arg(m_db.lastError().text()));
        cleanup();
        return;
    }

    {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("BEGIN"));
        m_inTransaction = true;
    }

    int s = 0;
    bool ok = false;

    // Step 1: Add the new column nullable so the ALTER does not have to
    // back-fill in a single shot on huge tables.
    ok = execStep(s++,
        QStringLiteral("Add column: sync_data.server_modified_at"),
        QStringLiteral(
            "ALTER TABLE sync_data ADD COLUMN IF NOT EXISTS server_modified_at BIGINT"));
    if (!ok) goto rollback;

    // Step 2: Backfill from updateddate. This preserves relative ordering
    // for existing rows so any client cursor based on the old updateddate
    // value remains valid against the new server_modified_at field.
    ok = execStep(s++,
        QStringLiteral("Backfill server_modified_at from updateddate"),
        QStringLiteral(
            "UPDATE sync_data SET server_modified_at = updateddate "
            "WHERE server_modified_at IS NULL"));
    if (!ok) goto rollback;

    // Step 3: Lock down NOT NULL now that every row has a value.
    ok = execStep(s++,
        QStringLiteral("Set NOT NULL on server_modified_at"),
        QStringLiteral(
            "ALTER TABLE sync_data ALTER COLUMN server_modified_at SET NOT NULL"));
    if (!ok) goto rollback;

    // Step 4: Trigger function — overwrites server_modified_at on every
    // INSERT and UPDATE so clients cannot influence its value.
    ok = execStep(s++,
        QStringLiteral("Create function: sync_data_stamp_server_time"),
        QStringLiteral(R"(
CREATE OR REPLACE FUNCTION sync_data_stamp_server_time()
RETURNS TRIGGER AS $$
BEGIN
    NEW.server_modified_at := (EXTRACT(EPOCH FROM clock_timestamp()) * 1000)::BIGINT;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql)"));
    if (!ok) goto rollback;

    // Step 5: Trigger.
    ok = execStep(s++,
        QStringLiteral("Create trigger: sync_data_stamp_server_time"),
        QStringLiteral(R"(
DO $$ BEGIN
    CREATE TRIGGER sync_data_stamp_server_time_trg
        BEFORE INSERT OR UPDATE ON sync_data
        FOR EACH ROW EXECUTE FUNCTION sync_data_stamp_server_time();
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
    if (!ok) goto rollback;

    // Step 6: New pull index on (userid, tablename, server_modified_at).
    ok = execStep(s++,
        QStringLiteral("Create index: idx_sync_data_pull on server_modified_at"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_sync_data_pull_v2 "
            "ON sync_data (userid, tablename, server_modified_at)"));
    if (!ok) goto rollback;

    // Step 7: Drop the old updateddate index — pulls no longer use it.
    ok = execStep(s++,
        QStringLiteral("Drop old index: idx_sync_data_pull"),
        QStringLiteral("DROP INDEX IF EXISTS idx_sync_data_pull"));
    if (!ok) goto rollback;

    {
        QSqlQuery q(m_db);
        if (!q.exec(QStringLiteral("COMMIT"))) {
            const QString err = q.lastError().text();
            m_inTransaction = false;
            cleanup();
            emit finished(false, QStringLiteral("Commit failed: %1").arg(err));
            return;
        }
        m_inTransaction = false;
    }

    cleanup();
    emit finished(true, QString());
    return;

rollback:
    if (m_inTransaction) {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("ROLLBACK"));
        m_inTransaction = false;
    }
    cleanup();
    emit finished(false, QStringLiteral("Upgrade aborted and rolled back."));
}
