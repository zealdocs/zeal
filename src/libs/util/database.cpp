// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2016 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "database.h"

#include <QMutexLocker>

#include <sqlite3.h>

namespace Zeal::Util {

namespace {
constexpr const char *ListTablesSql = "SELECT name"
                                      "  FROM"
                                      "    (SELECT * FROM sqlite_master UNION ALL"
                                      "    SELECT * FROM sqlite_temp_master)"
                                      "  WHERE type='table'"
                                      "  ORDER BY name";
constexpr const char *ListViewsSql = "SELECT name"
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

Database::Database(const QString &path, bool readOnly)
{
    if (sqlite3_initialize() != SQLITE_OK) {
        return;
    }

    const int flags = readOnly ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    if (sqlite3_open_v2(path.toUtf8().constData(), &m_db, flags, nullptr) != SQLITE_OK) {
        if (m_db != nullptr) {
            m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        }
        close();
        return;
    }

    // Docset databases are untrusted input. Defensive mode blocks writes to the schema and
    // shadow tables; an untrusted schema keeps SQL embedded in views, triggers and CHECK
    // constraints from calling functions or virtual tables that are not marked innocuous.
    // Zeal's own statements are unaffected. See https://www.sqlite.org/security.html.
    sqlite3_db_config(m_db, SQLITE_DBCONFIG_DEFENSIVE, 1, nullptr);
    sqlite3_db_config(m_db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, nullptr);
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
