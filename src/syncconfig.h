// Copyright (C) 2026 Paul McKinney
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
    int     batchSize           = 50;
    QString idColumn            = QStringLiteral("id");
    QString updatedDateColumn   = QStringLiteral("updateddate");
    QString syncDateColumn      = QStringLiteral("syncdate");
};

// Adaptive sync thresholds used by SyncLoopWorker.
// While sync completeness is below kCatchUpThreshold the loop uses the catch-up
// interval and batch size; once at or above the threshold it switches to
// steady-state values.
constexpr int    kCatchUpIntervalMs     = 15000;  // 15 s
constexpr int    kCatchUpBatchSize      = 150;
constexpr int    kSteadyStateIntervalMs = 30000;  // 30 s
constexpr int    kSteadyStateBatchSize  = 50;
constexpr double kCatchUpThreshold      = 0.90;   // 90 %
