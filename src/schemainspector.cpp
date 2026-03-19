#include "schemainspector.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonValue>
#include <QVariant>
#include <QDebug>

SchemaInspector::SchemaInspector(QSqlDatabase db)
    : m_db(db)
{
}

QList<ColumnInfo> SchemaInspector::getColumns(const QString &tableName)
{
    QList<ColumnInfo> columns;

    // PRAGMA table_info does not support bound parameters; tableName is developer-supplied.
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("PRAGMA table_info(\"%1\")").arg(tableName))) {
        qWarning() << "SchemaInspector::getColumns failed for" << tableName
                   << "-" << q.lastError().text();
        return columns;
    }

    while (q.next()) {
        ColumnInfo col;
        col.cid          = q.value(0).toInt();
        col.name         = q.value(1).toString();
        col.type         = q.value(2).toString().toUpper();
        col.notNull      = q.value(3).toBool();
        col.defaultValue = q.value(4).toString();
        col.isPrimaryKey = q.value(5).toBool();
        columns.append(col);
    }

    return columns;
}

bool SchemaInspector::hasRequiredColumns(const QString &tableName,
                                         const QString &idCol,
                                         const QString &updatedDateCol,
                                         const QString &syncDateCol,
                                         QString       &errorMsg)
{
    const auto columns = getColumns(tableName);
    QStringList upperNames;
    for (const auto &c : columns)
        upperNames << c.name.toUpper();

    QStringList missing;
    if (!upperNames.contains(idCol.toUpper()))          missing << idCol;
    if (!upperNames.contains(updatedDateCol.toUpper())) missing << updatedDateCol;
    if (!upperNames.contains(syncDateCol.toUpper()))    missing << syncDateCol;

    if (!missing.isEmpty()) {
        errorMsg = QStringLiteral("Table '%1' is missing required columns: %2")
                       .arg(tableName, missing.join(QStringLiteral(", ")));
        return false;
    }
    return true;
}

QJsonObject SchemaInspector::recordToJson(const QSqlRecord       &record,
                                           const QList<ColumnInfo> &columns,
                                           const QStringList       &excludeColumns)
{
    QStringList upperExcludes;
    for (const auto &e : excludeColumns)
        upperExcludes << e.toUpper();

    QJsonObject obj;
    for (const auto &col : columns) {
        if (upperExcludes.contains(col.name.toUpper()))
            continue;

        const QVariant val = record.value(col.name);

        if (val.isNull()) {
            obj.insert(col.name, QJsonValue::Null);
        } else if (col.type == QLatin1String("INTEGER")  ||
                   col.type == QLatin1String("BIGINT")   ||
                   col.type.startsWith(QLatin1String("INT"))) {
            obj.insert(col.name, val.toLongLong());
        } else if (col.type == QLatin1String("REAL")    ||
                   col.type == QLatin1String("FLOAT")   ||
                   col.type == QLatin1String("DOUBLE")  ||
                   col.type == QLatin1String("NUMERIC") ||
                   col.type.startsWith(QLatin1String("DECIMAL"))) {
            obj.insert(col.name, val.toDouble());
        } else if (col.type == QLatin1String("BLOB")) {
            obj.insert(col.name, QString::fromLatin1(val.toByteArray().toBase64()));
        } else {
            obj.insert(col.name, val.toString());
        }
    }

    return obj;
}

QVariant SchemaInspector::jsonValueToVariant(const QJsonValue &value, const QString &columnType)
{
    if (value.isNull() || value.isUndefined())
        return QVariant();

    const QString type = columnType.toUpper();

    if (type == QLatin1String("INTEGER") ||
        type == QLatin1String("BIGINT")  ||
        type.startsWith(QLatin1String("INT"))) {
        return QVariant(value.toVariant().toLongLong());
    }

    if (type == QLatin1String("REAL")    ||
        type == QLatin1String("FLOAT")   ||
        type == QLatin1String("DOUBLE")  ||
        type == QLatin1String("NUMERIC") ||
        type.startsWith(QLatin1String("DECIMAL"))) {
        return QVariant(value.toDouble());
    }

    if (type == QLatin1String("BLOB")) {
        return QVariant(QByteArray::fromBase64(value.toString().toLatin1()));
    }

    return QVariant(value.toString());
}
