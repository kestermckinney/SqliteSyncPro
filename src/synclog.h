// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QString>
#include <functional>

// Lightweight logging seam for the sync library.
//
// SqliteSyncProLib is built standalone (it is also linked into the separate
// admin host executable) and therefore has no dependency on the host app's
// QLogger. Instead of calling QLogger directly, failure points call
// SyncLog::error(); the host application installs a sink at startup that
// forwards those messages into its own logging system (syncerrors.log in
// ProjectNotes). If no sink is installed, messages fall back to qWarning().
namespace SyncLog
{
    using Sink = std::function<void(const QString &)>;

    // Install the sink that receives sync error messages. Pass a null
    // std::function to remove it. Thread-safe with respect to error().
    void setErrorSink(Sink sink);

    // Report a sync failure. Forwards to the installed sink, or qWarning()
    // when none is set. Safe to call from any thread.
    void error(const QString &message);
}
