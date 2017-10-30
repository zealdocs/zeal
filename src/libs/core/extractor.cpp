/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
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

#include "extractor.h"

#include <QDir>

#include <archive.h>
#include <archive_entry.h>

using namespace Zeal::Core;

Extractor::Extractor(QObject *parent) :
    QObject(parent)
{
}

void Extractor::extract(const QString &filePath, const QString &destination, const QString &root)
{
    ExtractInfo info = {
        this, // extractor
        archive_read_new(), // archiveHandle
        filePath, // filePath
        QFileInfo(filePath).size(), // totalBytes
        0 // extractedBytes
    };

    archive_read_support_filter_all(info.archiveHandle);
    archive_read_support_format_all(info.archiveHandle);

    int r = archive_read_open_filename(info.archiveHandle, qPrintable(filePath), 10240);
    if (r) {
        emit error(filePath, QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));
        return;
    }

    archive_read_extract_set_progress_callback(info.archiveHandle, &Extractor::progressCallback,
                                               &info);

    QDir destinationDir(destination);
    if (!root.isEmpty())
        destinationDir = destinationDir.filePath(root);

    // TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry;
    while (archive_read_next_header(info.archiveHandle, &entry) == ARCHIVE_OK) {
#ifndef Q_OS_WIN32
        QString pathname = QString::fromUtf8(archive_entry_pathname(entry));
#else
        // TODO: Remove once https://github.com/libarchive/libarchive/issues/587 is resolved.
        QString pathname = QString::fromWCharArray(archive_entry_pathname_w(entry));
#endif
        if (!root.isEmpty())
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        archive_entry_set_pathname(entry, qPrintable(destinationDir.filePath(pathname)));
        archive_read_extract(info.archiveHandle, entry, 0);
    }

    emit completed(filePath);
    archive_read_free(info.archiveHandle);
}

void Extractor::progressCallback(void *ptr)
{
    ExtractInfo *info = static_cast<ExtractInfo *>(ptr);

    const qint64 extractedBytes = archive_filter_bytes(info->archiveHandle, -1);
    if (extractedBytes == info->extractedBytes)
        return;

    info->extractedBytes = extractedBytes;

    emit info->extractor->progress(info->filePath, extractedBytes, info->totalBytes);
}
