// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "statement.h"

#include "sqlitedatabase.h"

#include <sqlite3.h>

namespace Zeal::Util {

Statement::Statement(SQLiteDatabase &db, const QString &sql)
{
    sqlite3 *const handle = db.handle();
    if (handle == nullptr) {
        m_lastError = QStringLiteral("Database is not open");
        return;
    }

    const void *pzTail = nullptr;
    const int res = sqlite3_prepare16_v2(handle,
                                         sql.constData(),
                                         static_cast<int>((sql.size() + 1) * sizeof(QChar)),
                                         &m_stmt,
                                         &pzTail);

    if (res != SQLITE_OK) {
        m_lastError = QString(static_cast<const QChar *>(sqlite3_errmsg16(handle)));
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
        return;
    }

    if (pzTail && !QString(static_cast<const QChar *>(pzTail)).trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Multiple statements in a single prepare are not supported");
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

Statement::~Statement()
{
    sqlite3_finalize(m_stmt);
}

bool Statement::isValid() const
{
    return m_stmt != nullptr;
}

void Statement::bindText(int index, QStringView value)
{
    if (m_stmt == nullptr) {
        return;
    }

    // SQLite binds SQL NULL when the data pointer is null, even with size 0.
    // Force a non-null pointer so an empty/null QString binds as empty text.
    static constexpr char16_t kEmpty = 0;
    const char16_t *data = value.utf16() != nullptr ? value.utf16() : &kEmpty;

    sqlite3_bind_text16(m_stmt, index, data, static_cast<int>(value.size() * sizeof(char16_t)), SQLITE_TRANSIENT);
}

void Statement::bindInt(int index, int value)
{
    if (m_stmt == nullptr) {
        return;
    }

    sqlite3_bind_int(m_stmt, index, value);
}

bool Statement::step()
{
    if (m_stmt == nullptr) {
        return false;
    }

    const int res = sqlite3_step(m_stmt);

    if (res == SQLITE_ROW) {
        return true;
    }

    if (res != SQLITE_DONE) {
        m_lastError = QString(static_cast<const QChar *>(sqlite3_errmsg16(sqlite3_db_handle(m_stmt))));
    }

    return false;
}

QVariant Statement::value(int index) const
{
    Q_ASSERT(index >= 0);

    if (m_stmt == nullptr || index < 0 || index >= sqlite3_data_count(m_stmt)) {
        return QVariant();
    }

    const int type = sqlite3_column_type(m_stmt, index);

    switch (type) {
    case SQLITE_INTEGER:
        return sqlite3_column_int64(m_stmt, index);
    case SQLITE_FLOAT:
        return sqlite3_column_double(m_stmt, index);
    case SQLITE_NULL:
        return QVariant();
    default:
        return QString(static_cast<const QChar *>(sqlite3_column_text16(m_stmt, index)),
                       sqlite3_column_bytes16(m_stmt, index) / sizeof(QChar));
    }
}

QString Statement::lastError() const
{
    return m_lastError;
}

} // namespace Zeal::Util
