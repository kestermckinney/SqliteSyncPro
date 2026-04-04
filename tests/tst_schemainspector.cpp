// Copyright (C) 2026 Paul McKinney
#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QJsonObject>

#include "schemainspector.h"

class tst_SchemaInspector : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase m_db;

    void createTestTable()
    {
        QSqlQuery q(m_db);
        q.exec("DROP TABLE IF EXISTS test_items");
        q.exec(R"(
            CREATE TABLE test_items (
                id          TEXT    PRIMARY KEY,
                name        TEXT    NOT NULL,
                quantity    INTEGER NOT NULL DEFAULT 0,
                price       REAL,
                updateddate INTEGER NOT NULL,
                syncdate    INTEGER
            )
        )");
    }

private slots:
    void initTestCase()
    {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "tst_schema");
        m_db.setDatabaseName(":memory:");
        QVERIFY(m_db.open());
    }

    void cleanupTestCase()
    {
        m_db.close();
        QSqlDatabase::removeDatabase("tst_schema");
    }

    void init()
    {
        createTestTable();
    }

    // ------------------------------------------------------------------

    void test_getColumns_returnsAllColumns()
    {
        SchemaInspector inspector(m_db);
        const auto cols = inspector.getColumns("test_items");
        QCOMPARE(cols.size(), 6);

        QStringList names;
        for (const auto &c : cols)
            names << c.name;

        QVERIFY(names.contains("id"));
        QVERIFY(names.contains("name"));
        QVERIFY(names.contains("quantity"));
        QVERIFY(names.contains("price"));
        QVERIFY(names.contains("updateddate"));
        QVERIFY(names.contains("syncdate"));
    }

    void test_getColumns_typesAreUppercased()
    {
        SchemaInspector inspector(m_db);
        const auto cols = inspector.getColumns("test_items");

        for (const auto &c : cols)
            QCOMPARE(c.type, c.type.toUpper());
    }

    void test_hasRequiredColumns_allPresent()
    {
        SchemaInspector inspector(m_db);
        QString err;
        QVERIFY(inspector.hasRequiredColumns("test_items", "id", "updateddate", "syncdate", err));
        QVERIFY(err.isEmpty());
    }

    void test_hasRequiredColumns_missingSync()
    {
        SchemaInspector inspector(m_db);
        QString err;
        QVERIFY(!inspector.hasRequiredColumns("test_items", "id", "updateddate", "MISSING_COL", err));
        QVERIFY(err.contains("MISSING_COL"));
    }

    void test_hasRequiredColumns_missingMultiple()
    {
        SchemaInspector inspector(m_db);
        QString err;
        QVERIFY(!inspector.hasRequiredColumns("test_items", "NO_ID", "updateddate", "NO_SYNC", err));
        QVERIFY(err.contains("NO_ID"));
        QVERIFY(err.contains("NO_SYNC"));
    }

    void test_recordToJson_serializesTypes()
    {
        QSqlQuery insert(m_db);
        insert.exec(R"(INSERT INTO test_items VALUES
            ('uuid-1', 'Widget', 5, 9.99, 1700000000000, NULL))");

        QSqlQuery select(m_db);
        select.exec("SELECT * FROM test_items WHERE id = 'uuid-1'");
        QVERIFY(select.next());

        SchemaInspector inspector(m_db);
        const auto cols = inspector.getColumns("test_items");
        const QJsonObject obj = inspector.recordToJson(select.record(), cols);

        QCOMPARE(obj["id"].toString(), QStringLiteral("uuid-1"));
        QCOMPARE(obj["name"].toString(), QStringLiteral("Widget"));
        QCOMPARE(obj["quantity"].toInt(), 5);
        QVERIFY(qFuzzyCompare(obj["price"].toDouble(), 9.99));
        QCOMPARE(obj["updateddate"].toVariant().toLongLong(), Q_INT64_C(1700000000000));
        QVERIFY(obj["syncdate"].isNull());
    }

    void test_recordToJson_excludeColumns()
    {
        QSqlQuery insert(m_db);
        insert.exec(R"(INSERT INTO test_items VALUES
            ('uuid-2', 'Gadget', 1, 19.99, 1700000000001, 1700000000001))");

        QSqlQuery select(m_db);
        select.exec("SELECT * FROM test_items WHERE id = 'uuid-2'");
        QVERIFY(select.next());

        SchemaInspector inspector(m_db);
        const auto cols = inspector.getColumns("test_items");
        const QJsonObject obj = inspector.recordToJson(select.record(), cols, {"syncdate"});

        QVERIFY(!obj.contains("syncdate"));
        QVERIFY(obj.contains("updateddate"));
    }

    void test_jsonValueToVariant_integer()
    {
        const QVariant v = SchemaInspector::jsonValueToVariant(
            QJsonValue(Q_INT64_C(1700000000000)), QStringLiteral("INTEGER"));
        QCOMPARE(v.toLongLong(), Q_INT64_C(1700000000000));
    }

    void test_jsonValueToVariant_real()
    {
        const QVariant v = SchemaInspector::jsonValueToVariant(
            QJsonValue(3.14), QStringLiteral("REAL"));
        QVERIFY(qFuzzyCompare(v.toDouble(), 3.14));
    }

    void test_jsonValueToVariant_text()
    {
        const QVariant v = SchemaInspector::jsonValueToVariant(
            QJsonValue(QStringLiteral("hello")), QStringLiteral("TEXT"));
        QCOMPARE(v.toString(), QStringLiteral("hello"));
    }

    void test_jsonValueToVariant_null()
    {
        const QVariant v = SchemaInspector::jsonValueToVariant(
            QJsonValue(QJsonValue::Null), QStringLiteral("TEXT"));
        QVERIFY(v.isNull());
    }
};

QTEST_MAIN(tst_SchemaInspector)
#include "tst_schemainspector.moc"
