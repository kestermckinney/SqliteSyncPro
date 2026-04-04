// Copyright (C) 2026 Paul McKinney
#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "syncengine.h"
#include "syncconfig.h"
#include "mockhttpclient.h"

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static const QString k_userId    = QStringLiteral("user-001");
static const QString k_pgTable   = QStringLiteral("sync_data");

static void createInvoiceTable(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("DROP TABLE IF EXISTS invoices");
    q.exec(R"(
        CREATE TABLE invoices (
            id             TEXT    PRIMARY KEY,
            invoice_number TEXT    NOT NULL,
            address        TEXT,
            updateddate    INTEGER NOT NULL,
            syncdate       INTEGER
        )
    )");
}

static void insertInvoice(QSqlDatabase &db,
                          const QString &id,
                          const QString &invoiceNumber,
                          const QString &address,
                          qint64 updatedDate,
                          QVariant syncDate = QVariant())
{
    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO invoices (id, invoice_number, address, updateddate, syncdate)
        VALUES (:id, :inv, :addr, :upd, :sync)
    )");
    q.bindValue(":id",   id);
    q.bindValue(":inv",  invoiceNumber);
    q.bindValue(":addr", address);
    q.bindValue(":upd",  updatedDate);
    q.bindValue(":sync", syncDate);
    q.exec();
}

static SyncTableConfig invoiceConfig()
{
    SyncTableConfig cfg;
    cfg.tableName         = QStringLiteral("invoices");
    cfg.batchSize         = 10;
    cfg.idColumn          = QStringLiteral("id");
    cfg.updatedDateColumn = QStringLiteral("updateddate");
    cfg.syncDateColumn    = QStringLiteral("syncdate");
    return cfg;
}

/** Wrap a row JSON object in the sync_data envelope the server would return. */
static QJsonObject makeSyncDataRow(const QString &recordId,
                                    qint64         updatedDate,
                                    const QJsonObject &rowData)
{
    QJsonObject row;
    row[QStringLiteral("userid")]      = k_userId;
    row[QStringLiteral("tablename")]   = QStringLiteral("invoices");
    row[QStringLiteral("id")]          = recordId;
    row[QStringLiteral("updateddate")] = updatedDate;
    row[QStringLiteral("jsonrowdata")] = rowData;
    return row;
}

/** JSON for a check-existence response (just id + updateddate). */
static QByteArray checkFound(const QString &recordId, qint64 updatedDate)
{
    QJsonArray arr;
    QJsonObject obj;
    obj[QStringLiteral("id")]          = recordId;
    obj[QStringLiteral("updateddate")] = updatedDate;
    arr.append(obj);
    return QJsonDocument(arr).toJson();
}

static QByteArray emptyArray() { return QByteArray("[]"); }

// ---------------------------------------------------------------------------

class tst_SyncEngine : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase   m_db;
    MockHttpClient m_http;

    SyncEngine *makeEngine()
    {
        auto *engine = new SyncEngine(this);
        engine->setDatabase(m_db);
        engine->setHttpClient(&m_http);
        engine->setUserId(k_userId);
        engine->setPostgresTableName(k_pgTable);
        return engine;
    }

private slots:

    void initTestCase()
    {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "tst_engine");
        m_db.setDatabaseName(":memory:");
        QVERIFY(m_db.open());
        m_http.setBaseUrl("https://test.example.com");
        m_http.setAuthToken("test-jwt");
    }

    void cleanupTestCase()
    {
        m_db.close();
        QSqlDatabase::removeDatabase("tst_engine");
    }

    void init()
    {
        createInvoiceTable(m_db);
        m_http.recordedRequests.clear();
        m_http.clearResponses();
    }

    // ------------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------------

    void test_missingRequiredColumns_returnsError()
    {
        QSqlQuery q(m_db);
        q.exec("DROP TABLE IF EXISTS bad_table");
        q.exec("CREATE TABLE bad_table (id TEXT PRIMARY KEY, name TEXT)");

        SyncTableConfig cfg;
        cfg.tableName = "bad_table";

        SyncEngine engine;
        engine.setDatabase(m_db);
        engine.setHttpClient(&m_http);
        engine.setPostgresTableName(k_pgTable);

        const SyncResult result = engine.synchronizeTable(cfg);
        QVERIFY(!result.success);
        QVERIFY(!result.errorMessage.isEmpty());
    }

    // ------------------------------------------------------------------
    // PUSH – new record (SYNCDATE IS NULL) → POST to sync_data
    // ------------------------------------------------------------------

    void test_push_newRecord_postsToSyncData()
    {
        insertInvoice(m_db, "local-uuid-1", "INV-001", "123 Main St",
                      Q_INT64_C(1000), QVariant());

        // Existence check: not found
        m_http.addJsonResponse(k_pgTable, emptyArray());
        // POST succeeds
        m_http.addJsonResponse(k_pgTable, {});

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPushed(), 1);

        // Verify the POST contained JSONROWDATA and the correct envelope fields
        bool foundPost = false;
        for (const auto &req : m_http.recordedRequests) {
            if (req.verb == "POST") {
                const QJsonObject body = QJsonDocument::fromJson(req.body).object();
                QCOMPARE(body["userid"].toString(),    k_userId);
                QCOMPARE(body["tablename"].toString(), QStringLiteral("invoices"));
                QCOMPARE(body["id"].toString(),        QStringLiteral("local-uuid-1"));
                QVERIFY(body.contains("updateddate"));
                QVERIFY(body.contains("jsonrowdata"));

                // syncdate must NOT appear in jsonrowdata
                const QJsonObject rowData = body["jsonrowdata"].toObject();
                QVERIFY(!rowData.contains("syncdate"));
                QVERIFY(rowData.contains("invoice_number"));
                foundPost = true;
                break;
            }
        }
        QVERIFY(foundPost);
    }

    void test_push_stampsSyncDateAfterSuccess()
    {
        insertInvoice(m_db, "stamp-uuid", "INV-002", "Addr",
                      Q_INT64_C(5000), QVariant());

        m_http.addJsonResponse(k_pgTable, emptyArray());
        m_http.addJsonResponse(k_pgTable, {});

        auto *engine = makeEngine();
        engine->synchronizeTable(invoiceConfig());

        QSqlQuery q(m_db);
        q.exec("SELECT syncdate, updateddate FROM invoices WHERE id = 'stamp-uuid'");
        QVERIFY(q.next());
        QCOMPARE(q.value("syncdate").toLongLong(), Q_INT64_C(5000));
    }

    // ------------------------------------------------------------------
    // PUSH – modified record (SYNCDATE < UPDATEDDATE) → PATCH sync_data
    // ------------------------------------------------------------------

    void test_push_modifiedRecord_patchesSyncData()
    {
        insertInvoice(m_db, "patch-uuid", "INV-003", "Old Addr",
                      Q_INT64_C(200), Q_INT64_C(100)); // UPDATEDDATE > SYNCDATE

        // Server has older version
        m_http.addJsonResponse(k_pgTable, checkFound("patch-uuid", Q_INT64_C(100)));
        // PATCH succeeds
        m_http.addJsonResponse(k_pgTable, {});

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPushed(), 1);

        bool foundPatch = false;
        for (const auto &req : m_http.recordedRequests) {
            if (req.verb == "PATCH") {
                const QJsonObject body = QJsonDocument::fromJson(req.body).object();
                // PATCH payload has updateddate and jsonrowdata only (no envelope fields)
                QVERIFY(body.contains("updateddate"));
                QVERIFY(body.contains("jsonrowdata"));
                QVERIFY(!body.contains("userid"));   // userid not re-sent on PATCH
                foundPatch = true;
                break;
            }
        }
        QVERIFY(foundPatch);
    }

    // ------------------------------------------------------------------
    // PUSH – server newer → conflict (skip, do not POST/PATCH)
    // ------------------------------------------------------------------

    void test_push_serverNewer_conflict()
    {
        insertInvoice(m_db, "conf-uuid", "INV-CONF", "Addr",
                      Q_INT64_C(100), QVariant());

        // Server has newer timestamp
        m_http.addJsonResponse(k_pgTable, checkFound("conf-uuid", Q_INT64_C(999)));
        // Pull: nothing
        m_http.addJsonResponse(k_pgTable, emptyArray());

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPushed(),    0);
        QCOMPARE(result.totalConflicts(), 1);

        // Verify no POST or PATCH was attempted
        for (const auto &req : m_http.recordedRequests) {
            QVERIFY(req.verb != "POST");
            QVERIFY(req.verb != "PATCH");
        }
    }

    // ------------------------------------------------------------------
    // PULL – server has new record → INSERT locally from JSONROWDATA
    // ------------------------------------------------------------------

    void test_pull_newServerRecord_insertsLocally()
    {
        // No dirty local records → push makes 0 HTTP calls.
        // Register pull response directly (no competing "push check" entry).
        QJsonObject rowData;
        rowData["invoice_number"] = "INV-SERVER-001";
        rowData["address"]        = "456 Server St";

        QJsonArray pullArr;
        pullArr.append(makeSyncDataRow("server-uuid-1", Q_INT64_C(2000), rowData));
        m_http.addJsonResponse(k_pgTable, QJsonDocument(pullArr).toJson());

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPulled(), 1);

        QSqlQuery q(m_db);
        q.exec("SELECT * FROM invoices WHERE id = 'server-uuid-1'");
        QVERIFY(q.next());
        QCOMPARE(q.value("invoice_number").toString(), QStringLiteral("INV-SERVER-001"));
        QCOMPARE(q.value("address").toString(),        QStringLiteral("456 Server St"));
        // syncdate is set to server updateddate
        QCOMPARE(q.value("syncdate").toLongLong(), Q_INT64_C(2000));
    }

    // ------------------------------------------------------------------
    // PULL – server is newer than local → UPDATE locally from JSONROWDATA
    // ------------------------------------------------------------------

    void test_pull_serverNewer_updatesLocally()
    {
        insertInvoice(m_db, "shared-uuid", "INV-OLD", "Old Addr",
                      Q_INT64_C(1000), Q_INT64_C(1000));

        // Push: nothing (already in sync)
        // Pull: server has newer version
        QJsonObject rowData;
        rowData["invoice_number"] = "INV-NEW";
        rowData["address"]        = "New Addr";

        QJsonArray pullArr;
        pullArr.append(makeSyncDataRow("shared-uuid", Q_INT64_C(2000), rowData));
        m_http.addJsonResponse(k_pgTable, QJsonDocument(pullArr).toJson());

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPulled(), 1);

        QSqlQuery q(m_db);
        q.exec("SELECT * FROM invoices WHERE id = 'shared-uuid'");
        QVERIFY(q.next());
        QCOMPARE(q.value("invoice_number").toString(), QStringLiteral("INV-NEW"));
        QCOMPARE(q.value("syncdate").toLongLong(),     Q_INT64_C(2000));
    }

    // ------------------------------------------------------------------
    // PULL – sync_data query uses TABLENAME filter (not table-specific endpoint)
    // ------------------------------------------------------------------

    void test_pull_queryFiltersByTableName()
    {
        m_http.addJsonResponse(k_pgTable, emptyArray()); // push check
        m_http.addJsonResponse(k_pgTable, emptyArray()); // pull

        auto *engine = makeEngine();
        engine->synchronizeTable(invoiceConfig());

        // Find the pull GET request and verify it filters on tablename
        bool foundTableFilter = false;
        for (const auto &req : m_http.recordedRequests) {
            if (req.verb == "GET" && req.url.contains("updateddate")) {
                QVERIFY(req.url.contains("tablename"));
                QVERIFY(req.url.contains("invoices"));
                foundTableFilter = true;
                break;
            }
        }
        QVERIFY(foundTableFilter);
    }

    // ------------------------------------------------------------------
    // Full round-trip: push new local + pull new server record
    // ------------------------------------------------------------------

    void test_fullRoundTrip()
    {
        insertInvoice(m_db, "local-only", "INV-LOCAL", "Local St",
                      Q_INT64_C(5000), QVariant());

        // Push: existence check → not found; POST needs no explicit response (default 200)
        m_http.addJsonResponse("id=eq.local-only", emptyArray());

        // Pull: different server record (use specific URL key to avoid push check collision)
        QJsonObject rowData;
        rowData["invoice_number"] = "INV-SERVER";
        rowData["address"]        = "Server Rd";

        QJsonArray pullArr;
        pullArr.append(makeSyncDataRow("server-only", Q_INT64_C(9999), rowData));
        m_http.addJsonResponse("updateddate=gte", QJsonDocument(pullArr).toJson());

        auto *engine = makeEngine();
        const SyncResult result = engine->synchronizeTable(invoiceConfig());

        QVERIFY(result.success);
        QCOMPARE(result.totalPushed(), 1);
        QCOMPARE(result.totalPulled(), 1);
    }
};

QTEST_MAIN(tst_SyncEngine)
#include "tst_syncengine.moc"
