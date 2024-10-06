// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanager.h"

#include "application.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>

#include <future>

using namespace Zeal::Core;

static Q_LOGGING_CATEGORY(log, "zeal.core.filemanager")

FileManager::FileManager(QObject *parent)
    : QObject(parent)
{
    // Ensure that cache location exists.
    // TODO: Check for errors.
    QDir().mkpath(Application::cacheLocation());
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
    }

    qCDebug(log, "Renamed '%s' to '%s'.", qPrintable(path), qPrintable(deletePath));

    std::future<bool> f = std::async(std::launch::async, [deletePath](){
        return QDir(deletePath).removeRecursively();
    });

    f.wait();

    if (!f.get()) {
        qCWarning(log, "Failed to remove '%s'.", qPrintable(deletePath));
    } else {
        qCDebug(log, "Removed '%s'.", qPrintable(deletePath));
    }

    return true;
}
