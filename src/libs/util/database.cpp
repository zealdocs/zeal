// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2016 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "database.h"

#include <QMutexLocker>

#include <sqlite3.h>

namespace Zeal::Util {

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
const auto ListCallback = [](void *ptr, int, char **data, char **) {
    static_cast<QStringList *>(ptr)->append(QString::fromUtf8(*data));
    return 0;
};
} // namespace

Database::Database(const QString &path)
{
    if (sqlite3_initialize() != SQLITE_OK) {
        return;
    }

    if (sqlite3_open16(path.constData(), &m_db) != SQLITE_OK) {
        if (m_db != nullptr) {
            m_lastError = QString(static_cast<const QChar *>(sqlite3_errmsg16(m_db)));
        }
        close();
    }
}

Database::~Database()
{
    close();
}

bool Database::isOpen() const
{
    return m_db != nullptr;
}

QStringList Database::tables()
{
    const QMutexLocker locker(&m_mutex);
    if (m_db == nullptr) {
        return {};
    }

    QStringList list;
    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, ListTablesSql, ListCallback, &list, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg != nullptr) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }

        return {};
    }

    return list;
}

QStringList Database::views()
{
    const QMutexLocker locker(&m_mutex);
    if (m_db == nullptr) {
        return {};
    }

    QStringList list;
    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, ListViewsSql, ListCallback, &list, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg != nullptr) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }

        return {};
    }

    return list;
}

bool Database::execute(const QString &sql)
{
    const QMutexLocker locker(&m_mutex);
    if (m_db == nullptr) {
        return false;
    }

    m_lastError.clear();

    char *errmsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql.toUtf8(), nullptr, nullptr, &errmsg);

    if (rc != SQLITE_OK) {
        if (errmsg != nullptr) {
            m_lastError = QString::fromUtf8(errmsg);
            sqlite3_free(errmsg);
        }

        return false;
    }

    return true;
}

QString Database::lastError() const
{
    // QString is not thread-safe for concurrent read+write.
    const QMutexLocker locker(&m_mutex);
    return m_lastError;
}

void Database::close()
{
    // Use the _v2 variant so that any prepared statements still alive at
    // shutdown defer the actual deallocation instead of returning SQLITE_BUSY
    // and leaking the connection.
    sqlite3_close_v2(m_db);
    m_db = nullptr;
}

sqlite3 *Database::handle() const
{
    return m_db;
}

} // namespace Zeal::Util
