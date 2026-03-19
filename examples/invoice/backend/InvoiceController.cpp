#include "InvoiceController.h"

#include "sqlitesyncpro.h"
#include "syncresult.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariantMap>
#include <QReadWriteLock>
#include <QRandomGenerator>
#include <QDebug>

#include <atomic>
#include <memory>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int kTestDurationMs   = 2 * 60 * 1000; // 2 minutes of mutations (no syncing)
static constexpr int kSyncIntervalMs   = 2000;           // ms between convergence sync rounds
static constexpr int kMutateMinMs      = 200;            // shortest pause between mutations
static constexpr int kMutateMaxMs      = 800;            // longest pause between mutations
static constexpr int kQuietRoundsNeeded = 2;             // consecutive idle rounds to declare done
static constexpr int kMaxConvergeRounds = 150;           // safety cap (~5 min at 2s/round)

// ---------------------------------------------------------------------------
// createInvoiceSchema
// ---------------------------------------------------------------------------

static bool createInvoiceSchema(QSqlDatabase &db)
{
    QSqlQuery q(db);

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoices (
            id             TEXT    PRIMARY KEY,
            invoice_number TEXT    NOT NULL,
            address        TEXT,
            updateddate    INTEGER NOT NULL,
            syncdate       INTEGER
        )
    )");
    if (!ok) {
        qWarning() << "createInvoiceSchema invoices:" << q.lastError().text();
        return false;
    }

    ok = q.exec(R"(
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
    if (!ok) {
        qWarning() << "createInvoiceSchema invoice_lines:" << q.lastError().text();
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// insertSampleData
// ---------------------------------------------------------------------------

static void insertSampleData(QSqlDatabase &db)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery q(db);

    const QString inv1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    q.prepare("INSERT OR IGNORE INTO invoices (id,invoice_number,address,updateddate,syncdate) "
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
    q.prepare("INSERT OR IGNORE INTO invoices (id,invoice_number,address,updateddate,syncdate) "
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
}

// ---------------------------------------------------------------------------
// readRecords – read invoices + lines for display in the ListView
// ---------------------------------------------------------------------------

static QVariantList readRecords(const QString &dbPath, const QString &connName)
{
    QVariantList result;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "readRecords open failed:" << db.lastError().text();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return result;
    }

    QSqlQuery q(db);

    q.exec("SELECT id, invoice_number, address, updateddate, syncdate "
           "FROM invoices ORDER BY invoice_number");
    while (q.next()) {
        QVariantMap row;
        row[QStringLiteral("rowType")]       = QStringLiteral("invoice");
        row[QStringLiteral("id")]            = q.value(0).toString();
        row[QStringLiteral("invoiceNumber")] = q.value(1).toString();
        row[QStringLiteral("address")]       = q.value(2).toString();
        row[QStringLiteral("updateddate")]   = q.value(3).toLongLong();
        const QVariant sd                    = q.value(4);
        row[QStringLiteral("synced")]        = !sd.isNull() && sd.toLongLong() == q.value(3).toLongLong();
        result.append(row);
    }

    q.exec("SELECT l.id, l.invoice_id, l.line_number, l.quantity, l.description, l.price, "
           "       l.updateddate, l.syncdate, i.invoice_number "
           "FROM invoice_lines l "
           "JOIN invoices i ON i.id = l.invoice_id "
           "ORDER BY i.invoice_number, l.line_number");
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
        const QVariant sd                    = q.value(7);
        row[QStringLiteral("synced")]        = !sd.isNull() && sd.toLongLong() == q.value(6).toLongLong();
        row[QStringLiteral("invoiceNumber")] = q.value(8).toString();
        result.append(row);
    }

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
    return result;
}

// ---------------------------------------------------------------------------
// mutateDatabaseThread
//
// Runs in its own QThread.  Randomly inserts new invoices/lines and updates
// existing records until *stop becomes true.  Every write is wrapped in a
// QWriteLocker so it coordinates safely with the sync engine's write lock.
// ---------------------------------------------------------------------------

static void mutateDatabaseThread(const QString                             &dbPath,
                                  const QString                             &connName,
                                  QReadWriteLock                            *lock,
                                  const std::shared_ptr<std::atomic<bool>>  &stop,
                                  const std::shared_ptr<std::atomic<int>>   &counter)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << connName << "mutator: could not open DB:" << db.lastError().text();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    QRandomGenerator rng = QRandomGenerator::securelySeeded();

    while (!stop->load(std::memory_order_relaxed)) {
        // Random delay before each mutation
        const int delay = kMutateMinMs + static_cast<int>(
            rng.bounded(static_cast<quint32>(kMutateMaxMs - kMutateMinMs)));
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
                      "(id, invoice_number, address, updateddate, syncdate) "
                      "VALUES (:id, :num, :addr, :upd, NULL)");
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
            if (inv.exec("SELECT ID FROM invoices ORDER BY RANDOM() LIMIT 1") && inv.next()) {
                const QString invId = inv.value(0).toString();

                QSqlQuery maxQ(db);
                maxQ.prepare("SELECT COALESCE(MAX(line_number), 0) "
                             "FROM invoice_lines WHERE invoice_id = :inv");
                maxQ.bindValue(":inv", invId);
                maxQ.exec();
                maxQ.next();
                const int nextLine = maxQ.value(0).toInt() + 1;

                QSqlQuery q(db);
                q.prepare("INSERT INTO invoice_lines "
                          "(id, invoice_id, line_number, quantity, description, "
                          " price, updateddate, syncdate) "
                          "VALUES (:id, :inv, :ln, :qty, :desc, :price, :upd, NULL)");
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
                      "SET address = :addr, updateddate = :upd, syncdate = NULL "
                      "WHERE id = (SELECT id FROM invoices ORDER BY RANDOM() LIMIT 1)");
            q.bindValue(":addr", QStringLiteral("%1 Updated Blvd").arg(rng.bounded(999u) + 1));
            q.bindValue(":upd",  now);
            q.exec();
            if (q.numRowsAffected() > 0)
                counter->fetch_add(1, std::memory_order_relaxed);

        } else {
            // UPDATE a random line's price
            QSqlQuery q(db);
            q.prepare("UPDATE invoice_lines "
                      "SET price = :price, updateddate = :upd, syncdate = NULL "
                      "WHERE id = (SELECT id FROM invoice_lines ORDER BY RANDOM() LIMIT 1)");
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
    qDebug().noquote() << QStringLiteral("[mutator] %1 exited — total mutations: %2")
                              .arg(connName).arg(counter->load());
}

// ---------------------------------------------------------------------------
// compareDatabases
//
// Compares all invoice and invoice_line records (excluding SYNCDATE) between
// two databases.  Returns an empty list on success, or one entry per mismatch.
// ---------------------------------------------------------------------------

static QStringList compareDatabases(const QString &srcPath, const QString &dstPath)
{
    QStringList mismatches;

    QSqlDatabase srcDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), "cmp_src");
    srcDb.setDatabaseName(srcPath);
    QSqlDatabase dstDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), "cmp_dst");
    dstDb.setDatabaseName(dstPath);

    auto cleanup = [&]() {
        srcDb.close(); srcDb = QSqlDatabase(); QSqlDatabase::removeDatabase("cmp_src");
        dstDb.close(); dstDb = QSqlDatabase(); QSqlDatabase::removeDatabase("cmp_dst");
    };

    if (!srcDb.open() || !dstDb.open()) {
        mismatches << QStringLiteral("Could not open databases for comparison");
        cleanup();
        return mismatches;
    }

    // Load all rows from a SELECT into a hash keyed by the ID column
    auto loadRows = [](QSqlDatabase &db, const QString &sql) {
        QHash<QString, QVariantMap> result;
        QSqlQuery q(db);
        if (!q.exec(sql)) return result;
        while (q.next()) {
            QVariantMap row;
            const QSqlRecord rec = q.record();
            for (int i = 0; i < rec.count(); ++i)
                row[rec.fieldName(i)] = q.value(i);
            result[row[QStringLiteral("id")].toString()] = row;
        }
        return result;
    };

    // ── invoices ──────────────────────────────────────────────────────────────
    const QString invSql  = QStringLiteral(
        "SELECT id, invoice_number, address, updateddate FROM invoices");
    auto srcInv = loadRows(srcDb, invSql);
    auto dstInv = loadRows(dstDb, invSql);

    for (auto it = srcInv.constBegin(); it != srcInv.constEnd(); ++it) {
        const QString shortId = it.key().left(8);
        if (!dstInv.contains(it.key())) {
            mismatches << QStringLiteral("invoices: %1 missing from dest").arg(shortId);
        } else {
            const auto &s = it.value(), &d = dstInv[it.key()];
            for (const QString &col : {QStringLiteral("invoice_number"),
                                       QStringLiteral("address"),
                                       QStringLiteral("updateddate")}) {
                if (s[col] != d[col])
                    mismatches << QStringLiteral("invoices: %1 col %2 differs "
                                                 "(src=%3 dst=%4)")
                                      .arg(shortId, col,
                                           s[col].toString(), d[col].toString());
            }
        }
    }
    for (auto it = dstInv.constBegin(); it != dstInv.constEnd(); ++it)
        if (!srcInv.contains(it.key()))
            mismatches << QStringLiteral("invoices: %1 missing from source")
                              .arg(it.key().left(8));

    // ── invoice_lines ─────────────────────────────────────────────────────────
    const QString lineSql = QStringLiteral(
        "SELECT id, invoice_id, line_number, quantity, description, price, updateddate "
        "FROM invoice_lines");
    auto srcLines = loadRows(srcDb, lineSql);
    auto dstLines = loadRows(dstDb, lineSql);

    for (auto it = srcLines.constBegin(); it != srcLines.constEnd(); ++it) {
        const QString shortId = it.key().left(8);
        if (!dstLines.contains(it.key())) {
            mismatches << QStringLiteral("invoice_lines: %1 missing from dest").arg(shortId);
        } else {
            const auto &s = it.value(), &d = dstLines[it.key()];
            for (const QString &col : {QStringLiteral("invoice_id"),
                                       QStringLiteral("line_number"),
                                       QStringLiteral("quantity"),
                                       QStringLiteral("description"),
                                       QStringLiteral("price"),
                                       QStringLiteral("updateddate")}) {
                if (s[col] != d[col])
                    mismatches << QStringLiteral("invoice_lines: %1 col %2 differs "
                                                 "(src=%3 dst=%4)")
                                      .arg(shortId, col,
                                           s[col].toString(), d[col].toString());
            }
        }
    }
    for (auto it = dstLines.constBegin(); it != dstLines.constEnd(); ++it)
        if (!srcLines.contains(it.key()))
            mismatches << QStringLiteral("invoice_lines: %1 missing from source")
                              .arg(it.key().left(8));

    cleanup();
    return mismatches;
}

// ---------------------------------------------------------------------------
// InvoiceController
// ---------------------------------------------------------------------------

InvoiceController::InvoiceController(QObject *parent)
    : QObject(parent)
{
}

InvoiceController::~InvoiceController()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void InvoiceController::setStatus(const QString &msg)
{
    m_statusText = msg;
    emit statusChanged();
}

void InvoiceController::setBusy(bool busy)
{
    m_busy = busy;
    emit busyChanged();
}

// ---------------------------------------------------------------------------
// runSync  –  5-minute concurrent stress test
//
// Architecture:
//   Orchestrator thread (m_workerThread):
//     • Authenticates source and dest SqliteSyncPro APIs via saved settings
//     • Starts source mutator thread and dest mutator thread
//     • Runs a sync loop every kSyncIntervalMs for kTestDurationMs
//     • Signals mutators to stop, waits for them
//     • Runs a final drain sync until both databases quiesce
//     • Compares all records; reports PASS or FAIL
//
//   Source mutator thread:
//     • Every 200–800 ms randomly INSERTs or UPDATEs in the source DB
//     • Uses sourceApi.databaseLock() to coordinate with the sync engine
//
//   Dest mutator thread:
//     • Same for the dest DB using destApi.databaseLock()
// ---------------------------------------------------------------------------

void InvoiceController::runSync()
{
    if (m_busy) return;
    setBusy(true);
    setStatus(QStringLiteral("Preparing test…"));

    const QString sourcePath = QDir::tempPath() + QStringLiteral("/sqsp_invoice_source.db");
    const QString destPath   = QDir::tempPath() + QStringLiteral("/sqsp_invoice_dest.db");
    QFile::remove(sourcePath);
    QFile::remove(destPath);

    // Create and seed both databases on the main thread before handing off
    auto openSetup = [](const QString &path, const QString &conn, bool seed) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        if (db.open()) {
            createInvoiceSchema(db);
            if (seed) insertSampleData(db);
            db.close();
        } else {
            qWarning() << conn << "setup failed:" << db.lastError().text();
        }
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(conn);
    };
    openSetup(sourcePath, QStringLiteral("source_setup"), true);
    openSetup(destPath,   QStringLiteral("dest_setup"),   false);

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    }

    m_workerThread = QThread::create([this, sourcePath, destPath]() {

        // ── Helper: report error and finish ───────────────────────────────────
        auto reportError = [this](const QString &msg) {
            QMetaObject::invokeMethod(this, [this, msg]() {
                setStatus(msg);
                setBusy(false);
            }, Qt::QueuedConnection);
        };

        // ── Authenticate and open ─────────────────────────────────────────────
        SqliteSyncPro sourceApi, destApi;

        if (!sourceApi.authenticateFromSettings()) {
            reportError(QStringLiteral("Source auth failed: ") + sourceApi.lastError()); return;
        }
        if (!destApi.authenticateFromSettings()) {
            reportError(QStringLiteral("Dest auth failed: ") + destApi.lastError()); return;
        }
        if (!sourceApi.openDatabase(sourcePath)) {
            reportError(QStringLiteral("Source DB open failed: ") + sourceApi.lastError()); return;
        }
        if (!destApi.openDatabase(destPath)) {
            reportError(QStringLiteral("Dest DB open failed: ") + destApi.lastError()); return;
        }

        sourceApi.addTable(QStringLiteral("invoices"),      50);
        sourceApi.addTable(QStringLiteral("invoice_lines"), 50);
        destApi.addTable  (QStringLiteral("invoices"),      50);
        destApi.addTable  (QStringLiteral("invoice_lines"), 50);

        QReadWriteLock *srcLock = sourceApi.databaseLock();
        QReadWriteLock *dstLock = destApi.databaseLock();

        // ── Phase 1: mutations only for kTestDurationMs (no syncing) ──────────
        auto stop       = std::make_shared<std::atomic<bool>>(false);
        auto srcCounter = std::make_shared<std::atomic<int>>(0);
        auto dstCounter = std::make_shared<std::atomic<int>>(0);

        QThread *srcMutThread = QThread::create(
            [srcPath = sourcePath, srcLock, stop, srcCounter]() {
                mutateDatabaseThread(srcPath, QStringLiteral("src_mutator"),
                                     srcLock, stop, srcCounter);
            });
        QThread *dstMutThread = QThread::create(
            [dstPath = destPath, dstLock, stop, dstCounter]() {
                mutateDatabaseThread(dstPath, QStringLiteral("dst_mutator"),
                                     dstLock, stop, dstCounter);
            });

        srcMutThread->start();
        dstMutThread->start();
        qDebug() << "[test] Phase 1: mutating for" << kTestDurationMs / 1000 << "s (no syncing)";

        const qint64 mutationEnd = QDateTime::currentMSecsSinceEpoch() + kTestDurationMs;
        while (QDateTime::currentMSecsSinceEpoch() < mutationEnd) {
            QThread::msleep(500);
            const qint64 remaining = qMax(0LL,
                (mutationEnd - QDateTime::currentMSecsSinceEpoch()) / 1000);
            const int srcMut = srcCounter->load(std::memory_order_relaxed);
            const int dstMut = dstCounter->load(std::memory_order_relaxed);
            QMetaObject::invokeMethod(this, [this, remaining, srcMut, dstMut]() {
                setStatus(QStringLiteral(
                    "Mutating — %1s left | src %2 changes  dst %3 changes")
                    .arg(remaining).arg(srcMut).arg(dstMut));
            }, Qt::QueuedConnection);
        }

        stop->store(true, std::memory_order_relaxed);
        srcMutThread->wait();
        dstMutThread->wait();
        delete srcMutThread;
        delete dstMutThread;

        const int totalSrcMut = srcCounter->load();
        const int totalDstMut = dstCounter->load();
        qDebug() << "[test] Phase 1 complete. src mutations:" << totalSrcMut
                 << " dst mutations:" << totalDstMut;

        // ── Phase 2: sync until both databases fully converge ─────────────────
        qDebug() << "[test] Phase 2: syncing to convergence";

        int syncRound      = 0;
        int totalPushed    = 0;
        int totalPulled    = 0;
        int totalConflicts = 0;
        int quietRounds    = 0;

        while (syncRound < kMaxConvergeRounds) {
            QThread::msleep(static_cast<unsigned long>(kSyncIntervalMs));

            const SyncResult sr = sourceApi.synchronize();
            const SyncResult dr = destApi.synchronize();
            ++syncRound;

            const int roundActivity = sr.totalPushed() + sr.totalPulled()
                                    + dr.totalPushed() + dr.totalPulled();
            totalPushed    += sr.totalPushed()    + dr.totalPushed();
            totalPulled    += sr.totalPulled()    + dr.totalPulled();
            totalConflicts += sr.totalConflicts() + dr.totalConflicts();

            qDebug().noquote()
                << QStringLiteral("[sync] round %1 | src +%2/-%3 | dst +%4/-%5 | quiet=%6")
                       .arg(syncRound)
                       .arg(sr.totalPushed()).arg(sr.totalPulled())
                       .arg(dr.totalPushed()).arg(dr.totalPulled())
                       .arg(quietRounds);

            QMetaObject::invokeMethod(this,
                [this, syncRound, totalPushed, totalPulled, totalConflicts, roundActivity]() {
                    setStatus(QStringLiteral(
                        "Syncing — round %1 | pushed %2  pulled %3  conflicts %4"
                        "%5")
                        .arg(syncRound)
                        .arg(totalPushed).arg(totalPulled).arg(totalConflicts)
                        .arg(roundActivity == 0
                             ? QStringLiteral("  (idle)")
                             : QStringLiteral("")));
                }, Qt::QueuedConnection);

            if (roundActivity == 0) {
                ++quietRounds;
                if (quietRounds >= kQuietRoundsNeeded)
                    break;
            } else {
                quietRounds = 0;
            }
        }

        qDebug() << "[test] Phase 2 complete after" << syncRound << "round(s)."
                 << (quietRounds >= kQuietRoundsNeeded ? "Converged." : "Safety cap hit.");

        // ── Read final state for display ──────────────────────────────────────
        const QVariantList srcRecords =
            readRecords(sourcePath, QStringLiteral("source_final_read"));
        const QVariantList dstRecords =
            readRecords(destPath,   QStringLiteral("dest_final_read"));

        QMetaObject::invokeMethod(this, [this, srcRecords, dstRecords]() {
            m_sourceRecords = srcRecords;
            emit sourceRecordsChanged();
            m_destRecords = dstRecords;
            emit destRecordsChanged();
        }, Qt::QueuedConnection);

        // ── Verify ────────────────────────────────────────────────────────────
        QMetaObject::invokeMethod(this, [this]() {
            setStatus(QStringLiteral("Verifying database consistency…"));
        }, Qt::QueuedConnection);

        const QStringList mismatches = compareDatabases(sourcePath, destPath);

        QString finalStatus;
        if (mismatches.isEmpty()) {
            finalStatus = QStringLiteral(
                "\u2713 PASS — %1 sync rounds | src %2 mutations | dst %3 mutations "
                "| pushed %4 | pulled %5 | conflicts %6 | all records identical")
                    .arg(syncRound).arg(totalSrcMut).arg(totalDstMut)
                    .arg(totalPushed).arg(totalPulled).arg(totalConflicts);
            qDebug() << "[verify] PASS — source records:" << srcRecords.size()
                     << "  dest records:" << dstRecords.size();
        } else {
            finalStatus = QStringLiteral(
                "\u2717 FAIL — %1 mismatch(es). First: %2")
                    .arg(mismatches.size()).arg(mismatches.first());
            qDebug() << "[verify] FAIL —" << mismatches.size() << "mismatch(es):";
            for (const auto &m : mismatches)
                qDebug() << "  " << m;
        }

        QMetaObject::invokeMethod(this, [this, finalStatus]() {
            setStatus(finalStatus);
            setBusy(false);
        }, Qt::QueuedConnection);
    });

    m_workerThread->setParent(this);
    connect(m_workerThread, &QThread::finished,
            m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, [this]() {
        m_workerThread = nullptr;
    });
    m_workerThread->start();
}
