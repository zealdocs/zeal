// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extractor.h"

#include <QDir>
#include <QLoggingCategory>

#include <archive.h>
#include <archive_entry.h>
#include <sys/stat.h>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.extractor")
} // namespace

Extractor::Extractor(QObject *parent)
    : QObject(parent)
{
}

void Extractor::extract(const QString &sourceFile, const QString &destination, const QString &root)
{
    ExtractInfo info = {.archiveHandle = archive_read_new(),
                        .filePath = sourceFile,
                        .totalBytes = QFileInfo(sourceFile).size(),
                        .extractedBytes = 0};

    archive_read_support_filter_all(info.archiveHandle);
    archive_read_support_format_all(info.archiveHandle);

    int rc = archive_read_open_filename(info.archiveHandle, qPrintable(sourceFile), 10240);
    if (rc != 0) {
        emit error(sourceFile, QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));
        archive_read_free(info.archiveHandle);

        return;
    }

    QDir destinationDir(destination);
    if (!root.isEmpty()) {
        destinationDir.setPath(destinationDir.filePath(root));
    }

    // Destination directory must be created before any other files.
    destinationDir.mkpath(QLatin1String("."));

    // TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry = nullptr;
    while (archive_read_next_header(info.archiveHandle, &entry) == ARCHIVE_OK) {
        // See https://github.com/libarchive/libarchive/issues/587 for more on UTF-8.
        QString pathname = QString::fromUtf8(archive_entry_pathname_utf8(entry));

        if (!root.isEmpty()) {
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        }

        const QString filePath = destinationDir.absoluteFilePath(pathname);

        const auto filetype = archive_entry_filetype(entry);
        if (filetype == S_IFDIR) {
            QDir().mkpath(filePath);
            continue;
        }

        if (filetype != S_IFREG) {
            qCWarning(log, "Unsupported filetype %d at '%s'.", filetype, qPrintable(pathname));
            continue;
        }

        // Create parent directories in case they are listed after their children (e.g. erl_tar).
        // See: https://github.com/zealdocs/zeal/issues/1393
        QDir().mkpath(QFileInfo(filePath).absolutePath());

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(log, "Cannot open file for writing at '%s'.", qPrintable(pathname));
            continue;
        }

        const void *buffer = nullptr;
        size_t size = 0;
        std::int64_t offset = 0;
        for (;;) {
            rc = archive_read_data_block(info.archiveHandle, &buffer, &size, &offset);
            if (rc != ARCHIVE_OK) {
                if (rc == ARCHIVE_EOF) {
                    break;
                }

                qCWarning(log, "Cannot read from archive: %s.", archive_error_string(info.archiveHandle));
                emit error(sourceFile, QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));

                archive_read_free(info.archiveHandle);

                return;
            }

            const auto toWrite = static_cast<qint64>(size);
            const qint64 written = file.write(static_cast<const char *>(buffer), toWrite);
            if (written != toWrite) {
                qCWarning(log, "Cannot write extracted file data for '%s'.", qPrintable(pathname));
                emit error(sourceFile, tr("Failed to write extracted file '%1'.").arg(pathname));
                archive_read_free(info.archiveHandle);
                return;
            }
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

} // namespace Zeal::Core
