// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2016 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_SQLITEDATABASE_H
#define ZEAL_UTIL_SQLITEDATABASE_H

#include <QStringList>
#include <QVariant>

struct sqlite3;
struct sqlite3_stmt;

namespace Zeal {
namespace Util {

class SQLiteDatabase
{
    Q_DISABLE_COPY(SQLiteDatabase)
public:
    explicit SQLiteDatabase(const QString &path);
    virtual ~SQLiteDatabase();

    bool isOpen() const;

    QStringList tables();
    QStringList views();

    bool prepare(const QString &sql);
    bool next();

    bool execute(const QString &sql);

    QVariant value(int index) const;

    QString lastError() const;

    sqlite3 *handle() const;

private:
    void close();
    void finalize();
    void updateLastError();

    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_stmt = nullptr;
    QString m_lastError;
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_SQLITEDATABASE_H
