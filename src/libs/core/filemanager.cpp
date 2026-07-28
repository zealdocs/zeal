// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanager.h"

#include "application.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <thread>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.filemanager")
} // namespace

FileManager::FileManager(QObject *parent)
    : QObject(parent)
{
    // Ensure that cache location exists.
    if (!QDir().mkpath(Application::cacheLocation())) {
        qCWarning(log, "Failed to create cache directory '%s'.", qPrintable(Application::cacheLocation()));
    }
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

    std::thread([deletePath]() {
        if (!QDir(deletePath).removeRecursively()) {
            qCWarning(log, "Failed to remove '%s'.", qPrintable(deletePath));
            return;
        }

        qCDebug(log, "Removed '%s'.", qPrintable(deletePath));
    }).detach();

    return true;
}

void FileManager::removePendingDeletions(const QString &path)
{
    if (path.isEmpty()) {
        qCWarning(log, "Cannot remove pending deletions without a storage path.");
        return;
    }

    std::thread([path]() {
        const QDir dir(path);
        QString storagePrefix = dir.canonicalPath();
        if (storagePrefix.isEmpty()) {
            qCWarning(log, "Cannot resolve the storage path '%s'.", qPrintable(path));
            return;
        }

        if (!storagePrefix.endsWith(QLatin1Char('/'))) {
            storagePrefix += QLatin1Char('/');
        }

        const QStringList names = dir.entryList({QStringLiteral("*.deleteme")}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &name : names) {
            const QString deletePath = dir.filePath(name);
            if (!QFileInfo(deletePath).canonicalFilePath().startsWith(storagePrefix)) {
                qCWarning(log, "Refusing to remove '%s' outside the storage directory.", qPrintable(deletePath));
                continue;
            }

            if (!QDir(deletePath).removeRecursively()) {
                qCWarning(log, "Failed to remove '%s'.", qPrintable(deletePath));
                continue;
            }

            qCDebug(log, "Removed '%s'.", qPrintable(deletePath));
        }
    }).detach();
}

} // namespace Zeal::Core
