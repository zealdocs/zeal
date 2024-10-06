/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2016 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#include "sqlitedatabase.h"

#include <sqlite3.h>

using namespace Zeal::Util;

namespace {
constexpr char ListTablesSql[] = "SELECT name"
                                 "  FROM"
                                 "    (SELECT * FROM sqlite_master UNION ALL"
                                 "    SELECT * FROM sqlite_temp_master)"
                                 "  WHERE type='table'"
                                 "  ORDER BY name";
constexpr char ListViewsSql[] = "SELECT name"
                                "  FROM"
                                "    (SELECT * FROM sqlite_master UNION ALL"
                                "    SELECT * FROM sqlite_temp_master)"
                                "  WHERE type='view'"
                                "  ORDER BY name";

// sqlite3_exec() callback used in tables() and views().
auto ListCallback = [](void *ptr, int, char **data, char **) {
    static_cast<QStringList *>(ptr)->append(QString::fromUtf8(data[0]));
    return 0;
};
} // namespace

SQLiteDatabase::SQLiteDatabase(const QString &path)
{
    if (sqlite3_initialize() != SQLITE_OK) {
        return;
    }

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
    if (m_db == nullptr) {
        return {};
    }

    QStringList list;
    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, ListTablesSql, ListCallback, &list, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }

        return {};
    }

    return list;
}

QStringList SQLiteDatabase::views()
{
    if (m_db == nullptr) {
        return {};
    }

    QStringList list;
    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, ListViewsSql, ListCallback, &list, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }

        return {};
    }

    return list;
}

bool SQLiteDatabase::prepare(const QString &sql)
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
    const int res = sqlite3_prepare16_v2(m_db,
                                         sql.constData(),
                                         (sql.size() + 1) * 2, // 2 = sizeof(QChar)
                                         &m_stmt,
                                         &pzTail);
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));

    if (res != SQLITE_OK) {
        // "Unable to execute statement"
        updateLastError();
        finalize();
        return false;
    }

    if (pzTail && !QString(static_cast<const QChar *>(pzTail)).trimmed().isEmpty()) {
        // Unable to execute multiple statements at a time
        updateLastError();
        finalize();
        return false;
    }

    return true;
}

bool SQLiteDatabase::next()
{
    if (m_stmt == nullptr) {
        return false;
    }

    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
    const int res = sqlite3_step(m_stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));

    switch (res) {
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

bool SQLiteDatabase::execute(const QString &sql)
{
    if (m_db == nullptr) {
        return false;
    }

    m_lastError.clear();

    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql.toUtf8(), nullptr, nullptr, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }
        return false;
    }

    return true;
}

QVariant SQLiteDatabase::value(int index) const
{
    Q_ASSERT(index >= 0);

    // sqlite3_data_count() returns 0 if m_stmt is nullptr.
    if (index >= sqlite3_data_count(m_stmt)) {
        return QVariant();
    }

    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
    const int type = sqlite3_column_type(m_stmt, index);

    QVariant ret;

    switch (type) {
    case SQLITE_INTEGER:
        ret = sqlite3_column_int64(m_stmt, index);
        break;
    case SQLITE_NULL:
        ret = QVariant();
        break;
    default:
        ret = QString(static_cast<const QChar *>(sqlite3_column_text16(m_stmt, index)),
                      sqlite3_column_bytes16(m_stmt, index) / 2); // 2 = sizeof(QChar)
        break;
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
    if (m_db == nullptr) {
        return;
    }

    m_lastError = QString(static_cast<const QChar *>(sqlite3_errmsg16(m_db)));
}

sqlite3 *SQLiteDatabase::handle() const
{
    return m_db;
}
