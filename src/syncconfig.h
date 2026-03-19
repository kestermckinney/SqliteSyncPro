#pragma once

#include <QString>

/**
 * Configuration for synchronizing a single table.
 *
 * Every table that participates in sync must have three columns:
 *   - idColumn        : unique record identifier (UUID text recommended)
 *   - updatedDateColumn: last-modified timestamp, stored as a UTC milliseconds epoch (INTEGER)
 *   - syncDateColumn  : tracks what has been synced to/from the server; NULL means the
 *                       record has never been pushed.  Set to updateddate after a
 *                       successful sync round-trip.  LOCAL ONLY – never sent to Postgres.
 */
struct SyncTableConfig
{
    QString tableName;
    int     batchSize           = 100;
    QString idColumn            = QStringLiteral("id");
    QString updatedDateColumn   = QStringLiteral("updateddate");
    QString syncDateColumn      = QStringLiteral("syncdate");
};
