// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlRecord>

struct ColumnInfo
{
    int     cid          = 0;
    QString name;
    QString type;           // SQLite declared type, uppercased
    bool    notNull      = false;
    QString defaultValue;
    bool    isPrimaryKey = false;
};

/**
 * Inspects a SQLite table's schema at runtime and converts rows to/from JSON.
 * Constructed with a copy of a QSqlDatabase connection handle.
 */
class SchemaInspector
{
public:
    explicit SchemaInspector(QSqlDatabase db);

    QList<ColumnInfo> getColumns(const QString &tableName);

    bool hasRequiredColumns(const QString &tableName,
                            const QString &idCol,
                            const QString &updatedDateCol,
                            const QString &syncDateCol,
                            QString       &errorMsg);

    /**
     * Serialize a QSqlRecord to a JSON object.
     * @param excludeColumns  Column names to omit (e.g. SYNCDATE, which is local-only).
     */
    QJsonObject recordToJson(const QSqlRecord      &record,
                             const QList<ColumnInfo> &columns,
                             const QStringList       &excludeColumns = {});

    /** Convert a JSON value back to a QVariant using the SQLite column type as a hint. */
    static QVariant jsonValueToVariant(const QJsonValue &value, const QString &columnType);

private:
    QSqlDatabase m_db;
};
