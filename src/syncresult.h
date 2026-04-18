// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QString>
#include <QList>

struct TableSyncResult
{
    QString tableName;
    int     pushed             = 0;
    int     pulled             = 0;
    int     conflicts          = 0;   // server was newer; server record wins, resolved in pull phase
    int     decryptionFailures = 0;   // records skipped because decryption failed (wrong key)
    bool    networkError       = false; // true when HTTP status == 0 (server unreachable)
    bool    authError          = false; // true when HTTP status == 401 (JWT expired)
    bool    success            = true;
    QString errorMessage;
    qint64  bytesPushed        = 0;     // bytes sent to server (upsert POST body)
    qint64  bytesPulled        = 0;     // bytes received from server (pull GET response)
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

    int totalDecryptionFailures() const
    {
        int n = 0;
        for (const auto &r : tableResults) n += r.decryptionFailures;
        return n;
    }

    bool hasNetworkError() const
    {
        for (const auto &r : tableResults)
            if (r.networkError) return true;
        return false;
    }

    bool hasAuthError() const
    {
        for (const auto &r : tableResults)
            if (r.authError) return true;
        return false;
    }

    qint64 totalBytesPushed() const
    {
        qint64 n = 0;
        for (const auto &r : tableResults) n += r.bytesPushed;
        return n;
    }

    qint64 totalBytesPulled() const
    {
        qint64 n = 0;
        for (const auto &r : tableResults) n += r.bytesPulled;
        return n;
    }
};
