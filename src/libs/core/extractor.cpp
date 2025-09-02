// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extractor.h"

#include <QDir>
#include <QLoggingCategory>

#include <archive.h>
#include <archive_entry.h>

#include <sys/stat.h>

using namespace Zeal::Core;

static Q_LOGGING_CATEGORY(log, "zeal.core.extractor")

Extractor::Extractor(QObject *parent)
    : QObject(parent)
{
}

void Extractor::extract(const QString &sourceFile, const QString &destination, const QString &root)
{
    ExtractInfo info = {
        archive_read_new(),           // archiveHandle
        sourceFile,                   // filePath
        QFileInfo(sourceFile).size(), // totalBytes
        0                             // extractedBytes
    };

    archive_read_support_filter_all(info.archiveHandle);
    archive_read_support_format_all(info.archiveHandle);

    int r = archive_read_open_filename(info.archiveHandle, qPrintable(sourceFile), 10240);
    if (r) {
        emit error(sourceFile, QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));
        return;
    }

    QDir destinationDir(destination);
    if (!root.isEmpty()) {
        destinationDir.setPath(destinationDir.filePath(root));
    }

    // Destination directory must be created before any other files.
    destinationDir.mkpath(QLatin1String("."));

    // TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry;
    while (archive_read_next_header(info.archiveHandle, &entry) == ARCHIVE_OK) {
        // See https://github.com/libarchive/libarchive/issues/587 for more on UTF-8.
        QString pathname = QString::fromUtf8(archive_entry_pathname_utf8(entry));

        if (!root.isEmpty()) {
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        }

        const QString filePath = destinationDir.absoluteFilePath(pathname);

        const auto filetype = archive_entry_filetype(entry);
        if (filetype == S_IFDIR) {
            QDir().mkpath(QFileInfo(filePath).absolutePath());
            continue;
        }

        if (filetype != S_IFREG) {
            qCWarning(log, "Unsupported filetype %d at '%s'.", filetype, qPrintable(pathname));
            continue;
        }

        QScopedPointer<QFile> file(new QFile(filePath));
        if (!file->open(QIODevice::WriteOnly)) {
            qCWarning(log, "Cannot open file for writing at '%s'.", qPrintable(pathname));
            continue;
        }

        const void *buffer;
        size_t size;
        std::int64_t offset;
        for (;;) {
            int rc = archive_read_data_block(info.archiveHandle, &buffer, &size, &offset);
            if (rc != ARCHIVE_OK) {
                if (rc == ARCHIVE_EOF) {
                    break;
                }

                qCWarning(log, "Cannot read from archive: %s.", archive_error_string(info.archiveHandle));
                emit error(sourceFile,
                           QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));
                return;
            }

            file->write(static_cast<const char *>(buffer), size);
        }

        emitProgress(info);
    }

    emit completed(sourceFile);
    archive_read_free(info.archiveHandle);
}

void Extractor::emitProgress(ExtractInfo &info)
{
    const qint64 extractedBytes = archive_filter_bytes(info.archiveHandle, -1);
    if (extractedBytes == info.extractedBytes) {
        return;
    }

    info.extractedBytes = extractedBytes;

    emit progress(info.filePath, extractedBytes, info.totalBytes);
}
