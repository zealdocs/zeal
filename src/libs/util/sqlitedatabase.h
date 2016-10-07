/****************************************************************************
**
** Copyright (C) 2016 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

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
public:
    explicit SQLiteDatabase(const QString &path);
    virtual ~SQLiteDatabase();

    bool isOpen() const;
    QStringList tables();

    bool execute(const QString &queryStr);
    bool next();

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
