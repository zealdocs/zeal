/****************************************************************************
**
** Copyright (C) 2017 Oleg Shparber
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

#include "filemanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <QtConcurrent>

using namespace Zeal::Core;

static Q_LOGGING_CATEGORY(log, "zeal.core.filemanager")

FileManager::FileManager(QObject *parent)
    : QObject(parent)
{
    // Ensure that cache location exists.
    // TODO: Check for errors.
    QDir().mkpath(cacheLocation());
}

bool FileManager::removeRecursively(const QString &path)
{
    qCDebug(log, "Removing '%s'...", qPrintable(path));

    if (!QFileInfo(path).isDir()) {
        qCWarning(log, "'%s' is not a directory.", qPrintable(path));
        return false;
    }

    const QString deletePath = QStringLiteral("%1.%2.deleteme")
            .arg(path, QString::number(QDateTime::currentMSecsSinceEpoch()));

    if (!QDir().rename(path, deletePath)) {
        qCWarning(log, "Failed to rename '%s' to '%s'.", qPrintable(path), qPrintable(deletePath));
        return false;
    } else {
        qCDebug(log, "Renamed '%s' to '%s'.", qPrintable(path), qPrintable(deletePath));
    }


    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>();
    connect(watcher, &QFutureWatcher<bool>::finished, [=] {
        if (!watcher->result()) {
            qCWarning(log, "Failed to remove '%s'.", qPrintable(deletePath));
        } else {
            qCDebug(log, "Removed '%s'.", qPrintable(deletePath));
        }

        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([deletePath] {
        return QDir(deletePath).removeRecursively();
    }));

    return true;
}

QString FileManager::cacheLocation()
{
#ifndef PORTABLE_BUILD
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    return QCoreApplication::applicationDirPath() + QLatin1String("/cache");
#endif
}
