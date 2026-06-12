// Copyright (C) 2026 Paul McKinney
#include "synclog.h"

#include <QDebug>
#include <QMutex>

namespace
{
    QMutex        g_sinkMutex;
    SyncLog::Sink g_sink;
}

namespace SyncLog
{
    void setErrorSink(Sink sink)
    {
        QMutexLocker locker(&g_sinkMutex);
        g_sink = std::move(sink);
    }

    void error(const QString &message)
    {
        Sink sink;
        {
            QMutexLocker locker(&g_sinkMutex);
            sink = g_sink;
        }

        if (sink)
            sink(message);
        else
            qWarning().noquote() << QStringLiteral("[SyncLog] ") + message;
    }
}
