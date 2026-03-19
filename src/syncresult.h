#pragma once

#include <QString>
#include <QList>

struct TableSyncResult
{
    QString tableName;
    int     pushed    = 0;
    int     pulled    = 0;
    int     conflicts = 0;   // server was newer; server record wins, resolved in pull phase
    bool    success   = true;
    QString errorMessage;
};

struct SyncResult
{
    bool    success      = true;
    QString errorMessage;
    QList<TableSyncResult> tableResults;

    int totalPushed() const
    {
        int n = 0;
        for (const auto &r : tableResults) n += r.pushed;
        return n;
    }

    int totalPulled() const
    {
        int n = 0;
        for (const auto &r : tableResults) n += r.pulled;
        return n;
    }

    int totalConflicts() const
    {
        int n = 0;
        for (const auto &r : tableResults) n += r.conflicts;
        return n;
    }
};
