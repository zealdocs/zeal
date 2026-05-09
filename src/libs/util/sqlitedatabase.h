// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2016 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_SQLITEDATABASE_H
#define ZEAL_UTIL_SQLITEDATABASE_H

#include <QMutex>
#include <QStringList>

struct sqlite3;

namespace Zeal::Util {

class SQLiteDatabase
{
    Q_DISABLE_COPY_MOVE(SQLiteDatabase)
public:
    explicit SQLiteDatabase(const QString &path);
    virtual ~SQLiteDatabase();

    bool isOpen() const;

    QStringList tables();
    QStringList views();

    bool execute(const QString &sql);

    QString lastError() const;

    sqlite3 *handle() const;

private:
    void close();

    mutable QMutex m_mutex;

    sqlite3 *m_db = nullptr;

    QString m_lastError;
};

} // namespace Zeal::Util

#endif // ZEAL_UTIL_SQLITEDATABASE_H
