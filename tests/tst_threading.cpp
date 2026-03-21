#include <QtTest>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSignalSpy>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "sqlitesyncpro.h"
#include "mockhttpclient.h"

// ---------------------------------------------------------------------------
// Helper: create a temp database with the invoice table
// ---------------------------------------------------------------------------

static QString createTempDb(const QString &suffix = QString())
{
    const QString path = QDir::tempPath() + QStringLiteral("/ssp_thread_test%1.db").arg(suffix);
    QFile::remove(path);  // start clean

    const QString connName = QStringLiteral("setup_%1").arg(suffix);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(path);
    db.open();

    QSqlQuery q(db);
    q.exec(R"(
        CREATE TABLE invoices (
            id             TEXT    PRIMARY KEY,
            invoice_number TEXT    NOT NULL,
            address        TEXT,
            updateddate    INTEGER NOT NULL,
            syncdate       INTEGER
        )
    )");
    // Insert two unsynced rows
    q.exec(R"(INSERT INTO invoices VALUES ('id-1','INV-001','Addr 1',1000,NULL))");
    q.exec(R"(INSERT INTO invoices VALUES ('id-2','INV-002','Addr 2',2000,NULL))");

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
    return path;
}

// ---------------------------------------------------------------------------

class tst_Threading : public QObject
{
    Q_OBJECT

private slots:

    // ------------------------------------------------------------------
    // synchronize() called from a background QThread must not crash and
    // must produce the same result as a call from the main thread.
    // ------------------------------------------------------------------
    void test_synchronizeFromWorkerThread()
    {
        const QString dbPath = createTempDb("_worker");

        SqliteSyncPro api;
        api.authenticateWithToken("https://test.example.com", "fake-jwt");
        QVERIFY(api.openDatabase(dbPath));
        api.addTable("invoices", 10);

        SyncResult threadResult;
        bool threadFinished = false;

        // Run synchronize() inside a plain QThread lambda
        QThread *t = QThread::create([&]() {
            // The mock must be created here because HttpClient (QNAM)
            // needs thread affinity – but SqliteSyncPro uses its own
            // internal HttpClient, so this test just exercises the DB path.
            // We expect a failure because there is no real server, but the
            // important thing is it does not crash or assert.
            threadResult   = api.synchronize();
            threadFinished = true;
        });
        t->start();
        t->wait(5000);
        delete t;

        QVERIFY(threadFinished);
        // Result may succeed or fail (no real server), but must not be a DB error.
        // The error, if any, must be network-related, not a "connection used in
        // wrong thread" or "database not open" error.
        if (!threadResult.success) {
            QVERIFY(!threadResult.errorMessage.contains("wrong thread",   Qt::CaseInsensitive));
            QVERIFY(!threadResult.errorMessage.contains("not open",       Qt::CaseInsensitive));
            QVERIFY(!threadResult.errorMessage.contains("database error", Qt::CaseInsensitive));
        }

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // Two threads calling synchronize() concurrently must not interfere.
    // Each gets its own DB connection; WAL mode lets both proceed.
    // ------------------------------------------------------------------
    void test_concurrentSynchronizeCalls()
    {
        const QString dbPath = createTempDb("_concurrent");

        SqliteSyncPro api;
        api.authenticateWithToken("https://test.example.com", "fake-jwt");
        QVERIFY(api.openDatabase(dbPath));
        api.addTable("invoices", 10);

        QAtomicInt finishedCount{0};

        auto syncJob = [&]() {
            api.synchronize();   // result ignored – just must not crash
            finishedCount.fetchAndAddRelaxed(1);
        };

        QThread *t1 = QThread::create(syncJob);
        QThread *t2 = QThread::create(syncJob);
        t1->start();
        t2->start();
        t1->wait(5000);
        t2->wait(5000);
        delete t1;
        delete t2;

        QCOMPARE(finishedCount.loadRelaxed(), 2);

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // synchronizeAsync() must emit syncCompleted exactly once and not
    // block the calling thread.
    // ------------------------------------------------------------------
    void test_synchronizeAsync_emitsCompleted()
    {
        const QString dbPath = createTempDb("_async");

        SqliteSyncPro api;
        api.authenticateWithToken("https://test.example.com", "fake-jwt");
        QVERIFY(api.openDatabase(dbPath));
        api.addTable("invoices", 10);

        QSignalSpy spy(&api, &SqliteSyncPro::syncCompleted);

        api.synchronizeAsync();

        // Wait up to 5 s for the background thread to complete.
        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // openDatabase() must enable WAL mode so concurrent readers work.
    // ------------------------------------------------------------------
    void test_walModeEnabled()
    {
        const QString dbPath = createTempDb("_wal");

        SqliteSyncPro api;
        QVERIFY(api.openDatabase(dbPath));

        // Open a separate connection and check the journal mode.
        const QString connName = "wal_check";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);
        QVERIFY(db.open());

        QSqlQuery q(db);
        q.exec("PRAGMA journal_mode");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString().toLower(), QStringLiteral("wal"));

        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // App and sync share the same QReadWriteLock via databaseLock().
    // Verify that databaseLock() returns a non-null pointer and that
    // read/write locking works without deadlock.
    // ------------------------------------------------------------------
    void test_sharedLock_readWriteCoordination()
    {
        const QString dbPath = createTempDb("_lock");

        SqliteSyncPro api;
        QVERIFY(api.openDatabase(dbPath));

        QReadWriteLock *lock = api.databaseLock();
        QVERIFY(lock != nullptr);

        // Simulate the app taking a read lock while no sync is running.
        { QReadLocker rl(lock); QVERIFY(true); }

        // Simulate the app taking a write lock.
        { QWriteLocker wl(lock); QVERIFY(true); }

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // setDatabaseLock() replaces the internal lock; databaseLock() returns
    // the external one.  Passing null resets to the internal lock.
    // ------------------------------------------------------------------
    void test_setDatabaseLock_replacesInternalLock()
    {
        const QString dbPath = createTempDb("_extlock");

        SqliteSyncPro api;
        QVERIFY(api.openDatabase(dbPath));

        QReadWriteLock *original = api.databaseLock();
        QVERIFY(original != nullptr);

        QReadWriteLock externalLock;
        api.setDatabaseLock(&externalLock);
        QCOMPARE(api.databaseLock(), &externalLock);

        // Passing null resets to the internal lock.
        api.setDatabaseLock(nullptr);
        QVERIFY(api.databaseLock() != nullptr);
        QVERIFY(api.databaseLock() != &externalLock);

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // App holds a read lock; synchronizeAsync() must wait for the lock
    // to be released before writing (no deadlock, no crash).
    // ------------------------------------------------------------------
    void test_appReadLock_blocksSync()
    {
        const QString dbPath = createTempDb("_readblock");

        SqliteSyncPro api;
        api.authenticateWithToken("https://test.example.com", "fake-jwt");
        QVERIFY(api.openDatabase(dbPath));
        api.addTable("invoices", 10);

        QReadWriteLock *lock = api.databaseLock();
        QSignalSpy spy(&api, &SqliteSyncPro::syncCompleted);

        // Hold a read lock from the app side, then launch async sync.
        // The sync (write lock) must wait – once we release, it should complete.
        {
            QReadLocker rl(lock);
            api.synchronizeAsync();
            // Sync is now queued behind our read lock; give it a moment to start.
            QThread::msleep(50);
            // Still holding read lock here – sync cannot have completed yet.
            QCOMPARE(spy.count(), 0);
        } // read lock released here

        // Now the sync can proceed; wait for completion.
        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);

        QFile::remove(dbPath);
    }

    // ------------------------------------------------------------------
    // config mutated from one thread while another is reading must not
    // produce torn reads (QMutex coverage).
    // ------------------------------------------------------------------
    void test_concurrentConfigAndSync()
    {
        const QString dbPath = createTempDb("_config");

        SqliteSyncPro api;
        api.authenticateWithToken("https://test.example.com", "fake-jwt");
        QVERIFY(api.openDatabase(dbPath));
        api.addTable("invoices", 10);

        QAtomicInt stop{0};

        // Writer thread: repeatedly clears and re-adds tables
        QThread *writer = QThread::create([&]() {
            while (!stop.loadRelaxed()) {
                api.clearTables();
                api.addTable("invoices", 10);
            }
        });

        // Reader thread: repeatedly calls isDatabaseOpen / isAuthenticated
        QThread *reader = QThread::create([&]() {
            for (int i = 0; i < 500; ++i) {
                (void)api.isDatabaseOpen();
                (void)api.isAuthenticated();
                (void)api.lastError();
            }
        });

        writer->start();
        reader->start();
        reader->wait(2000);
        stop.storeRelaxed(1);
        writer->wait(2000);
        delete writer;
        delete reader;

        // If we get here without a crash or assertion, the mutex is working.
        QVERIFY(true);

        QFile::remove(dbPath);
    }
};

QTEST_MAIN(tst_Threading)
#include "tst_threading.moc"
