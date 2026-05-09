// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_STATEMENT_H
#define ZEAL_UTIL_STATEMENT_H

#include <QString>
#include <QStringView>
#include <QVariant>

struct sqlite3_stmt;

namespace Zeal::Util {

class SQLiteDatabase;

// Short-lived RAII wrapper around sqlite3_stmt. Owned by the caller; the
// underlying sqlite3 connection in SQLiteDatabase is shared.
class Statement
{
    Q_DISABLE_COPY_MOVE(Statement)
public:
    Statement(SQLiteDatabase &db, const QString &sql);
    ~Statement();

    bool isValid() const;

    // SQLite uses 1-based indexing for bind, 0-based for value/column.
    void bindText(int index, QStringView value);
    void bindInt(int index, int value);

    bool step();
    QVariant value(int index) const;

    QString lastError() const;

private:
    sqlite3_stmt *m_stmt = nullptr;
    QString m_lastError;
};

// Escape SQL LIKE wildcards (%, _) and the escape character itself so the
// pattern only matches literal occurrences. Pair with `ESCAPE '\\'` in SQL.
QString escapeLikePattern(QStringView s);

} // namespace Zeal::Util

#endif // ZEAL_UTIL_STATEMENT_H
