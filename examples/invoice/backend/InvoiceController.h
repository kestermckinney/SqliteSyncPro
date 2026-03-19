#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QThread;

/**
 * InvoiceController – QML-facing backend for the invoice sync demo.
 *
 * Manages two in-memory (temp-file) SQLite databases:
 *   Source  – pre-populated with sample invoices; simulates "Device A" that pushes.
 *   Dest    – starts empty; simulates "Device B" that pulls.
 *
 * Both databases sync against the same PostgREST server using SqliteSyncPro.
 * Connection settings are managed via the SqliteSyncPro settings dialog
 * (Settings… button → showSettings()).
 */
class InvoiceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString statusText    READ statusText    NOTIFY statusChanged)
    Q_PROPERTY(bool    busy          READ isBusy        NOTIFY busyChanged)
    Q_PROPERTY(QVariantList sourceRecords READ sourceRecords NOTIFY sourceRecordsChanged)
    Q_PROPERTY(QVariantList destRecords   READ destRecords   NOTIFY destRecordsChanged)

public:
    explicit InvoiceController(QObject *parent = nullptr);
    ~InvoiceController() override;

    QString      statusText()    const { return m_statusText;    }
    bool         isBusy()        const { return m_busy;          }
    QVariantList sourceRecords() const { return m_sourceRecords; }
    QVariantList destRecords()   const { return m_destRecords;   }

    Q_INVOKABLE void runSync();

signals:
    void statusChanged();
    void busyChanged();
    void sourceRecordsChanged();
    void destRecordsChanged();

private:
    void setStatus(const QString &msg);
    void setBusy(bool busy);

    QString      m_statusText;
    bool         m_busy = false;

    QVariantList m_sourceRecords;
    QVariantList m_destRecords;

    QThread *m_workerThread = nullptr;
};
