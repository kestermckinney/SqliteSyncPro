#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <memory>

class SqliteSyncPro;
class QThread;
class QTimer;

/**
 * InvoiceController – backend for the Invoice Sync Demo.
 *
 * Manages two SqliteSyncPro instances (Client A and Client B) that
 * continuously sync two SQLite databases against the same PostgREST server.
 *
 * While the sync loops are running, two mutator threads independently
 * generate random invoice inserts and updates in each database to demonstrate
 * live convergence.
 *
 * Call startSync() to configure both APIs from saved settings and begin.
 * Call shutdown() before closing; the stopped() signal fires once everything
 * has exited cleanly.
 */
class InvoiceController : public QObject
{
    Q_OBJECT

public:
    explicit InvoiceController(QObject *parent = nullptr);
    ~InvoiceController() override;

    bool    isBusy()     const { return m_busy;    }
    bool    isRunning()  const { return m_running; }
    QString statusText() const { return m_status;  }

    QVariantList clientARecords() const { return m_clientARecords; }
    QVariantList clientBRecords() const { return m_clientBRecords; }

    /** Initialise both APIs and start sync + mutator threads. */
    void startSync();

    /** Request all threads to stop gracefully.  Emits stopped() when done. */
    void shutdown();

signals:
    void busyChanged();
    void statusChanged();
    void clientARecordsChanged();
    void clientBRecordsChanged();

    /** Emitted once both sync loops have stopped cleanly. */
    void stopped();

private slots:
    void onClientARowChanged(const QString &tableName, const QString &id);
    void onClientBRowChanged(const QString &tableName, const QString &id);
    void onSyncStopped();
    void refreshClientADisplay();
    void refreshClientBDisplay();
    void updateStatus();

private:
    void setStatus(const QString &msg);
    void setBusy(bool busy);

    static QVariantList readRecords(const QString &dbPath);

    SqliteSyncPro *m_clientAApi = nullptr;
    SqliteSyncPro *m_clientBApi = nullptr;

    // Mutator threads – one per database.
    QThread *m_mutatorThreadA = nullptr;
    QThread *m_mutatorThreadB = nullptr;

    // Shared stop flag written by shutdown(), read by both mutator threads.
    std::shared_ptr<std::atomic<bool>> m_mutateStop;

    // Mutation counters read by the status timer.
    std::shared_ptr<std::atomic<int>> m_clientAChangeCount;
    std::shared_ptr<std::atomic<int>> m_clientBChangeCount;

    bool    m_busy         = false;
    bool    m_running      = false;
    bool    m_shuttingDown = false;
    int     m_stopCount    = 0;
    QString m_status;

    QVariantList m_clientARecords;
    QVariantList m_clientBRecords;

    // Debounce timers so a burst of rowChanged signals causes one refresh.
    QTimer *m_clientARefreshTimer = nullptr;
    QTimer *m_clientBRefreshTimer = nullptr;

    // Periodic status timer showing live mutation counts.
    QTimer *m_statusTimer = nullptr;
};
