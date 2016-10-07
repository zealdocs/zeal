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

#include "sqlitedatabase.h"

#include <sqlite3.h>

using namespace Zeal::Util;

SQLiteDatabase::SQLiteDatabase(const QString &path)
{
    if (sqlite3_initialize() != SQLITE_OK)
        return;

    if (sqlite3_open16(path.constData(), &m_db) != SQLITE_OK) {
        updateLastError();
        close();
    }
}

SQLiteDatabase::~SQLiteDatabase()
{
    finalize();
    close();
}

bool SQLiteDatabase::isOpen() const
{
    return m_db != nullptr;
}

QStringList SQLiteDatabase::tables()
{
    Q_ASSERT(!m_stmt);  // FIXME: usage between other execute/next calls (Zeal doesn't need it now)

    QStringList res;
    if (!isOpen())
        return res;

    const QString sql = QLatin1String("SELECT name FROM sqlite_master WHERE type='table' UNION ALL "
                                      "SELECT name FROM sqlite_temp_master WHERE type='table'");

    if (!sql.isEmpty() && execute(sql)) {
        while(next())
            res.append(value(0).toString());
    }

    return res;
}

bool SQLiteDatabase::execute(const QString &queryStr)
{
    if (m_db == nullptr) {
        return false;
    }

    if (m_stmt != nullptr) {
        finalize();
    }

    m_lastError.clear();

    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
    const void *pzTail = nullptr;
    const int res = sqlite3_prepare16_v2(m_db, queryStr.constData(),
                                         (queryStr.size() + 1) * sizeof(QChar), &m_stmt, &pzTail);
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));

    if (res != SQLITE_OK) {
        // "Unable to execute statement"
        updateLastError();
        finalize();
        return false;
    } else if (pzTail && !QString(reinterpret_cast<const QChar *>(pzTail)).trimmed().isEmpty()) {
        // Unable to execute multiple statements at a time
        updateLastError();
        finalize();
        return false;
    }

    return true;
}

bool SQLiteDatabase::next()
{
    if (m_stmt == nullptr)
        return false;

    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
    const int res = sqlite3_step(m_stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));

    switch(res) {
    case SQLITE_ROW:
        return true;
    case SQLITE_DONE:
    case SQLITE_CONSTRAINT:
    case SQLITE_ERROR:
    case SQLITE_MISUSE:
    case SQLITE_BUSY:
    default:
        updateLastError();
    }

    return false;
}

QVariant SQLiteDatabase::value(int index) const
{
    Q_ASSERT(index >= 0);

    // sqlite3_data_count() returns 0 if m_stmt is nullptr.
    if (index >= sqlite3_data_count(m_stmt))
        return QVariant();

    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
    const int type = sqlite3_column_type(m_stmt, index);

    QVariant ret;

    switch (type) {
    case SQLITE_INTEGER:
        ret = sqlite3_column_int64(m_stmt, index);
    case SQLITE_NULL:
        ret = QVariant(QVariant::String);
    default:
        ret = QString(reinterpret_cast<const QChar *>(sqlite3_column_text16(m_stmt, index)),
                      sqlite3_column_bytes16(m_stmt, index) / sizeof(QChar));
    }
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));
    return ret;
}

QString SQLiteDatabase::lastError() const
{
    return m_lastError;
}

void SQLiteDatabase::close()
{
    sqlite3_close(m_db);
    m_db = nullptr;
}

void SQLiteDatabase::finalize()
{
    sqlite3_finalize(m_stmt);
    m_stmt = nullptr;
}

void SQLiteDatabase::updateLastError()
{
    if (!m_db)
        return;

    m_lastError = QString(reinterpret_cast<const QChar *>(sqlite3_errmsg16(m_db)));
}

sqlite3 *SQLiteDatabase::handle() const
{
    return m_db;
}
