#include "InvoiceController.h"
#include "../InvoiceSettingsDialog.h"
#include "sqlitesyncpro.h"

#include <QDateTime>
#include <QReadLocker>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>
#include <QWriteLocker>
#include <QDebug>

#include <atomic>
#include <memory>

// ---------------------------------------------------------------------------
// Schema helpers
// ---------------------------------------------------------------------------

static void createInvoiceSchema(const QString &dbPath)
{
    const QString connName = QStringLiteral("inv_schema_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
#ifdef QT_DEBUG
        qWarning() << "createInvoiceSchema: cannot open" << dbPath;
#endif
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    QSqlQuery q(db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoices (
            id             TEXT    PRIMARY KEY,
            invoice_number TEXT    NOT NULL,
            address        TEXT,
            updateddate    INTEGER NOT NULL,
            syncdate       INTEGER
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoice_lines (
            id           TEXT    PRIMARY KEY,
            invoice_id   TEXT    NOT NULL,
            line_number  INTEGER NOT NULL,
            quantity     REAL    NOT NULL,
            description  TEXT,
            price        REAL    NOT NULL,
            updateddate  INTEGER NOT NULL,
            syncdate     INTEGER
        )
    )");

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
}

static bool isDatabaseEmpty(const QString &dbPath)
{
    const QString connName = QStringLiteral("inv_check_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    bool empty = true;
    if (db.open()) {
        QSqlQuery q(db);
        q.exec("SELECT COUNT(*) FROM invoices");
        if (q.next()) empty = (q.value(0).toInt() == 0);
        db.close();
    }
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
    return empty;
}

static void seedData(const QString &dbPath)
{
    const QString connName = QStringLiteral("inv_seed_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery q(db);

    const QString inv1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    q.prepare("INSERT OR IGNORE INTO invoices "
              "(id,invoice_number,address,updateddate,syncdate) "
              "VALUES (:id,:num,:addr,:upd,NULL)");
    q.bindValue(":id",   inv1);
    q.bindValue(":num",  "INV-0001");
    q.bindValue(":addr", "123 Main Street, Springfield");
    q.bindValue(":upd",  now);
    q.exec();

    q.prepare("INSERT OR IGNORE INTO invoice_lines "
              "(id,invoice_id,line_number,quantity,description,price,updateddate,syncdate) "
              "VALUES (:id,:inv,:ln,:qty,:desc,:price,:upd,NULL)");
    q.bindValue(":id",    QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(":inv",   inv1); q.bindValue(":ln",    1);
    q.bindValue(":qty",   2.0);  q.bindValue(":desc",  "Widget A");
    q.bindValue(":price", 49.99); q.bindValue(":upd",  now);
    q.exec();

    q.bindValue(":id",    QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(":inv",   inv1); q.bindValue(":ln",    2);
    q.bindValue(":qty",   1.0);  q.bindValue(":desc",  "Widget B");
    q.bindValue(":price", 24.50); q.bindValue(":upd",  now + 1);
    q.exec();

    const QString inv2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    q.prepare("INSERT OR IGNORE INTO invoices "
              "(id,invoice_number,address,updateddate,syncdate) "
              "VALUES (:id,:num,:addr,:upd,NULL)");
    q.bindValue(":id",   inv2);
    q.bindValue(":num",  "INV-0002");
    q.bindValue(":addr", "456 Oak Avenue, Shelbyville");
    q.bindValue(":upd",  now + 2);
    q.exec();

    q.prepare("INSERT OR IGNORE INTO invoice_lines "
              "(id,invoice_id,line_number,quantity,description,price,updateddate,syncdate) "
              "VALUES (:id,:inv,:ln,:qty,:desc,:price,:upd,NULL)");
    q.bindValue(":id",    QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(":inv",   inv2); q.bindValue(":ln",    1);
    q.bindValue(":qty",   10.0); q.bindValue(":desc",  "Bulk Gadget");
    q.bindValue(":price", 5.99); q.bindValue(":upd",  now + 2);
    q.exec();

    q.bindValue(":id",    QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(":inv",   inv2); q.bindValue(":ln",    2);
    q.bindValue(":qty",   3.0);  q.bindValue(":desc",  "Deluxe Gadget");
    q.bindValue(":price", 19.99); q.bindValue(":upd",  now + 3);
    q.exec();

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
}

// ---------------------------------------------------------------------------
// mutateDatabaseThread
//
// Runs in its own QThread.  Randomly inserts new invoices / lines and updates
// existing records until *stop becomes true.  Every write uses QWriteLocker
// so it coordinates safely with the sync engine's write lock.
// ---------------------------------------------------------------------------

static void mutateDatabaseThread(const QString                            &dbPath,
                                  const QString                            &connBaseName,
                                  QReadWriteLock                           *lock,
                                  const std::shared_ptr<std::atomic<bool>> &stop,
                                  const std::shared_ptr<std::atomic<int>>  &counter)
{
    static constexpr int kMinMs = 300;
    static constexpr int kMaxMs = 900;

    const QString connName = QStringLiteral("%1_%2")
                                 .arg(connBaseName,
                                      QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
#ifdef QT_DEBUG
        qWarning() << connName << "mutator: cannot open DB:" << db.lastError().text();
#endif
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    QRandomGenerator rng = QRandomGenerator::securelySeeded();

    while (!stop->load(std::memory_order_relaxed)) {
        const int delay = kMinMs + static_cast<int>(
            rng.bounded(static_cast<quint32>(kMaxMs - kMinMs)));
        QThread::msleep(static_cast<unsigned long>(delay));

        if (stop->load(std::memory_order_relaxed))
            break;

        const qint64 now    = QDateTime::currentMSecsSinceEpoch();
        const int    action = static_cast<int>(rng.bounded(4u));

        QWriteLocker wl(lock);

        if (action == 0) {
            // INSERT a new invoice
            QSqlQuery q(db);
            q.prepare("INSERT INTO invoices "
                      "(id,invoice_number,address,updateddate,syncdate) "
                      "VALUES (:id,:num,:addr,:upd,NULL)");
            q.bindValue(":id",   QUuid::createUuid().toString(QUuid::WithoutBraces));
            q.bindValue(":num",  QStringLiteral("INV-%1").arg(rng.bounded(9000u) + 1000));
            q.bindValue(":addr", QStringLiteral("%1 Maple St, Anytown")
                                     .arg(rng.bounded(999u) + 1));
            q.bindValue(":upd",  now);
            if (q.exec())
                counter->fetch_add(1, std::memory_order_relaxed);

        } else if (action == 1) {
            // INSERT a new line into a random existing invoice
            QSqlQuery inv(db);
            if (inv.exec("SELECT id FROM invoices ORDER BY RANDOM() LIMIT 1") && inv.next()) {
                const QString invId = inv.value(0).toString();

                QSqlQuery maxQ(db);
                maxQ.prepare("SELECT COALESCE(MAX(line_number),0) "
                             "FROM invoice_lines WHERE invoice_id = :inv");
                maxQ.bindValue(":inv", invId);
                maxQ.exec();
                maxQ.next();
                const int nextLine = maxQ.value(0).toInt() + 1;

                QSqlQuery q(db);
                q.prepare("INSERT INTO invoice_lines "
                          "(id,invoice_id,line_number,quantity,description,"
                          " price,updateddate,syncdate) "
                          "VALUES (:id,:inv,:ln,:qty,:desc,:price,:upd,NULL)");
                q.bindValue(":id",    QUuid::createUuid().toString(QUuid::WithoutBraces));
                q.bindValue(":inv",   invId);
                q.bindValue(":ln",    nextLine);
                q.bindValue(":qty",   static_cast<double>(rng.bounded(10u) + 1));
                q.bindValue(":desc",  QStringLiteral("Item-%1").arg(rng.bounded(100u)));
                q.bindValue(":price", static_cast<double>(rng.bounded(10000u)) / 100.0);
                q.bindValue(":upd",   now);
                if (q.exec())
                    counter->fetch_add(1, std::memory_order_relaxed);
            }

        } else if (action == 2) {
            // UPDATE a random invoice's address
            QSqlQuery q(db);
            q.prepare("UPDATE invoices "
                      "SET address=:addr, updateddate=:upd, syncdate=NULL "
                      "WHERE id=(SELECT id FROM invoices ORDER BY RANDOM() LIMIT 1)");
            q.bindValue(":addr", QStringLiteral("%1 Updated Blvd")
                                     .arg(rng.bounded(999u) + 1));
            q.bindValue(":upd",  now);
            q.exec();
            if (q.numRowsAffected() > 0)
                counter->fetch_add(1, std::memory_order_relaxed);

        } else {
            // UPDATE a random line's price
            QSqlQuery q(db);
            q.prepare("UPDATE invoice_lines "
                      "SET price=:price, updateddate=:upd, syncdate=NULL "
                      "WHERE id=(SELECT id FROM invoice_lines ORDER BY RANDOM() LIMIT 1)");
            q.bindValue(":price", static_cast<double>(rng.bounded(10000u)) / 100.0);
            q.bindValue(":upd",   now);
            q.exec();
            if (q.numRowsAffected() > 0)
                counter->fetch_add(1, std::memory_order_relaxed);
        }
    }

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
#ifdef QT_DEBUG
    qDebug().noquote()
        << QStringLiteral("[mutator] %1 exited — total mutations: %2")
               .arg(connBaseName).arg(counter->load());
#endif
}

// ---------------------------------------------------------------------------
// readRecords – read invoices + lines for the tree display
// ---------------------------------------------------------------------------

QVariantList InvoiceController::readRecords(const QString &dbPath)
{
    QVariantList result;

    const QString connName = QStringLiteral("inv_read_%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return result;
    }

    QSqlQuery q(db);
    q.exec("SELECT id,invoice_number,address,updateddate,syncdate "
           "FROM invoices ORDER BY invoice_number");
    while (q.next()) {
        QVariantMap row;
        row[QStringLiteral("rowType")]       = QStringLiteral("invoice");
        row[QStringLiteral("id")]            = q.value(0).toString();
        row[QStringLiteral("invoiceNumber")] = q.value(1).toString();
        row[QStringLiteral("address")]       = q.value(2).toString();
        row[QStringLiteral("updateddate")]   = q.value(3).toLongLong();
        const QVariant sd = q.value(4);
        row[QStringLiteral("synced")] =
            !sd.isNull() && sd.toLongLong() == q.value(3).toLongLong();
        result.append(row);
    }

    q.exec("SELECT l.id,l.invoice_id,l.line_number,l.quantity,l.description,l.price,"
           "       l.updateddate,l.syncdate,i.invoice_number "
           "FROM invoice_lines l "
           "JOIN invoices i ON i.id=l.invoice_id "
           "ORDER BY i.invoice_number,l.line_number");
    while (q.next()) {
        QVariantMap row;
        row[QStringLiteral("rowType")]       = QStringLiteral("line");
        row[QStringLiteral("id")]            = q.value(0).toString();
        row[QStringLiteral("invoiceId")]     = q.value(1).toString();
        row[QStringLiteral("lineNumber")]    = q.value(2).toInt();
        row[QStringLiteral("quantity")]      = q.value(3).toDouble();
        row[QStringLiteral("description")]   = q.value(4).toString();
        row[QStringLiteral("price")]         = q.value(5).toDouble();
        row[QStringLiteral("updateddate")]   = q.value(6).toLongLong();
        const QVariant sd = q.value(7);
        row[QStringLiteral("synced")] =
            !sd.isNull() && sd.toLongLong() == q.value(6).toLongLong();
        row[QStringLiteral("invoiceNumber")] = q.value(8).toString();
        result.append(row);
    }

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
    return result;
}

// ---------------------------------------------------------------------------
// InvoiceController
// ---------------------------------------------------------------------------

InvoiceController::InvoiceController(QObject *parent)
    : QObject(parent)
    , m_clientAApi(new SqliteSyncPro(this))
    , m_clientBApi(new SqliteSyncPro(this))
{
    // Debounce timers: collect rowChanged bursts into a single display refresh.
    m_clientARefreshTimer = new QTimer(this);
    m_clientARefreshTimer->setSingleShot(true);
    m_clientARefreshTimer->setInterval(300);
    connect(m_clientARefreshTimer, &QTimer::timeout,
            this, &InvoiceController::refreshClientADisplay);

    m_clientBRefreshTimer = new QTimer(this);
    m_clientBRefreshTimer->setSingleShot(true);
    m_clientBRefreshTimer->setInterval(300);
    connect(m_clientBRefreshTimer, &QTimer::timeout,
            this, &InvoiceController::refreshClientBDisplay);

    // Status timer: update mutation counts every 500 ms while running.
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(500);
    connect(m_statusTimer, &QTimer::timeout,
            this, &InvoiceController::updateStatus);
}

InvoiceController::~InvoiceController()
{
    // Ensure mutator threads are stopped before the databases are closed.
    if (m_mutateStop)
        m_mutateStop->store(true, std::memory_order_relaxed);
    if (m_mutatorThreadA) { m_mutatorThreadA->wait(); }
    if (m_mutatorThreadB) { m_mutatorThreadB->wait(); }
}

void InvoiceController::setStatus(const QString &msg)
{
    m_status = msg;
    emit statusChanged();
}

void InvoiceController::setBusy(bool busy)
{
    m_busy = busy;
    emit busyChanged();
}

// ---------------------------------------------------------------------------
// startSync
// ---------------------------------------------------------------------------

void InvoiceController::startSync()
{
    if (m_running) {
        setStatus(QStringLiteral("Already running."));
        return;
    }

    setBusy(true);
    m_stopCount    = 0;
    m_shuttingDown = false;
    setStatus(QStringLiteral("Configuring…"));

    // Apply saved settings to both APIs.
    InvoiceSettingsDialog::applyToApis(m_clientAApi, m_clientBApi);

    const QString pathA = m_clientAApi->databasePath();
    const QString pathB = m_clientBApi->databasePath();

    if (pathA.isEmpty() || pathB.isEmpty()) {
        setStatus(QStringLiteral("Database paths not configured. Open Settings… first."));
        setBusy(false);
        return;
    }

    // Prepare schemas; seed Client A if empty.
    createInvoiceSchema(pathA);
    if (isDatabaseEmpty(pathA))
        seedData(pathA);
    createInvoiceSchema(pathB);

    // Connect signals before initialising (UniqueConnection avoids duplicates on re-start).
    connect(m_clientAApi, &SqliteSyncPro::rowChanged,
            this, &InvoiceController::onClientARowChanged, Qt::UniqueConnection);
    connect(m_clientBApi, &SqliteSyncPro::rowChanged,
            this, &InvoiceController::onClientBRowChanged, Qt::UniqueConnection);
    connect(m_clientAApi, &SqliteSyncPro::syncStopped,
            this, &InvoiceController::onSyncStopped, Qt::UniqueConnection);
    connect(m_clientBApi, &SqliteSyncPro::syncStopped,
            this, &InvoiceController::onSyncStopped, Qt::UniqueConnection);

    // Initialise both APIs (opens DB, discovers tables, authenticates, starts loop).
    setStatus(QStringLiteral("Initialising Client A…"));
    if (!m_clientAApi->initialize()) {
        setStatus(QStringLiteral("Client A init failed: ") + m_clientAApi->lastError());
        setBusy(false);
        return;
    }

    setStatus(QStringLiteral("Initialising Client B…"));
    if (!m_clientBApi->initialize()) {
        setStatus(QStringLiteral("Client B init failed: ") + m_clientBApi->lastError());
        m_clientAApi->shutdown();
        setBusy(false);
        return;
    }

    // Set up shared stop flag and mutation counters.
    m_mutateStop        = std::make_shared<std::atomic<bool>>(false);
    m_clientAChangeCount = std::make_shared<std::atomic<int>>(0);
    m_clientBChangeCount = std::make_shared<std::atomic<int>>(0);

    // Start mutator threads – each writes to its own database under its lock.
    auto stopA    = m_mutateStop;
    auto countA   = m_clientAChangeCount;
    auto *lockA   = m_clientAApi->databaseLock();

    m_mutatorThreadA = QThread::create([pathA, lockA, stopA, countA]() {
        mutateDatabaseThread(pathA, QStringLiteral("mutator_A"), lockA, stopA, countA);
    });
    connect(m_mutatorThreadA, &QThread::finished,
            m_mutatorThreadA, &QObject::deleteLater);
    connect(m_mutatorThreadA, &QThread::finished, this, [this]() {
        m_mutatorThreadA = nullptr;
    });

    auto stopB  = m_mutateStop;
    auto countB = m_clientBChangeCount;
    auto *lockB = m_clientBApi->databaseLock();

    m_mutatorThreadB = QThread::create([pathB, lockB, stopB, countB]() {
        mutateDatabaseThread(pathB, QStringLiteral("mutator_B"), lockB, stopB, countB);
    });
    connect(m_mutatorThreadB, &QThread::finished,
            m_mutatorThreadB, &QObject::deleteLater);
    connect(m_mutatorThreadB, &QThread::finished, this, [this]() {
        m_mutatorThreadB = nullptr;
    });

    m_mutatorThreadA->start();
    m_mutatorThreadB->start();

    m_running      = true;
    m_stopCount    = 0;
    m_shuttingDown = false;
    setBusy(false);

    m_statusTimer->start();

    // Populate initial display.
    refreshClientADisplay();
    refreshClientBDisplay();
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------

void InvoiceController::shutdown()
{
    if (!m_running) {
        emit stopped();
        return;
    }

    m_shuttingDown = true;
    m_stopCount    = 0;
    m_statusTimer->stop();
    setStatus(QStringLiteral("Shutting down…"));

    // Stop mutator threads first so they don't generate more work for the sync.
    if (m_mutateStop)
        m_mutateStop->store(true, std::memory_order_relaxed);

    // Request sync loops to stop (they emit syncStopped when done).
    m_clientAApi->shutdown();
    m_clientBApi->shutdown();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void InvoiceController::onClientARowChanged(const QString &, const QString &)
{
    m_clientARefreshTimer->start();
}

void InvoiceController::onClientBRowChanged(const QString &, const QString &)
{
    m_clientBRefreshTimer->start();
}

void InvoiceController::onSyncStopped()
{
    if (!m_running && !m_shuttingDown)
        return;

    ++m_stopCount;
    if (m_stopCount >= 2) {
        m_running = false;
        setStatus(QStringLiteral("Sync stopped."));
        if (m_shuttingDown)
            emit stopped();
    }
}

void InvoiceController::updateStatus()
{
    const int a = m_clientAChangeCount ? m_clientAChangeCount->load(std::memory_order_relaxed) : 0;
    const int b = m_clientBChangeCount ? m_clientBChangeCount->load(std::memory_order_relaxed) : 0;
    setStatus(QStringLiteral("Syncing — Client A: %1 local changes  |  Client B: %2 local changes")
                  .arg(a).arg(b));
}

void InvoiceController::refreshClientADisplay()
{
    const QString path = m_clientAApi->databasePath();
    if (path.isEmpty()) return;

    QVariantList records;
    {
        QReadLocker rl(m_clientAApi->databaseLock());
        records = readRecords(path);
    }
    m_clientARecords = records;
    emit clientARecordsChanged();
}

void InvoiceController::refreshClientBDisplay()
{
    const QString path = m_clientBApi->databasePath();
    if (path.isEmpty()) return;

    QVariantList records;
    {
        QReadLocker rl(m_clientBApi->databaseLock());
        records = readRecords(path);
    }
    m_clientBRecords = records;
    emit clientBRecordsChanged();
}
