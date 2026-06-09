// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extractor.h"

#include <util/tarixarchive.h>

#include <QDir>
#include <QLoggingCategory>
#include <QThread>

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
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, sourceFile, destination, root] {
            extract(sourceFile, destination, root);
        }, Qt::QueuedConnection);
        return;
    }

    if (extractEntries(sourceFile, destination, root, QString())) {
        emit completed(sourceFile);
    }
}

// Installs a docset that serves documents directly from its tarix archive:
// only metadata entries are extracted, while the archive and its index are
// placed under Contents/Resources for on-demand reads.
void Extractor::installTarixDocset(const QString &sourceFile,
                                   const QString &indexArchiveFile,
                                   const QString &destination,
                                   const QString &root)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, sourceFile, indexArchiveFile, destination, root] {
            installTarixDocset(sourceFile, indexArchiveFile, destination, root);
        }, Qt::QueuedConnection);
        return;
    }

    const QDir docsetDir(QDir(destination).filePath(root));
    const QString indexPath = docsetDir.filePath(QStringLiteral("Contents/Resources/tarixIndex.db"));

    bool usable = extractTarixIndex(indexArchiveFile, indexPath);
    if (usable) {
        // Read back a document so a stale or mismatched index falls back to extraction.
        const Util::TarixArchive archive(sourceFile, indexPath);
        usable = archive.isOpen() && archive.verify();
    }

    if (!usable) {
        qCWarning(log, "Unusable tarix index for '%s', falling back to extraction.", qPrintable(sourceFile));
        QFile::remove(indexPath);
        extract(sourceFile, destination, root);
        return;
    }

    if (!extractEntries(sourceFile, destination, root, QStringLiteral("Contents/Resources/Documents"))) {
        QFile::remove(indexPath);
        return;
    }

    const QString archivePath = docsetDir.filePath(QStringLiteral("Contents/Resources/tarix.tgz"));
    if (!QFile::rename(sourceFile, archivePath) && !QFile::copy(sourceFile, archivePath)) {
        emit error(sourceFile, tr("Failed to move archive to '%1'.").arg(archivePath));
        return;
    }

    emit completed(sourceFile);
}

// Strips the leading directory in the archive; entries whose stripped path
// starts with `skipPrefix` are not written to disk.
bool Extractor::extractEntries(const QString &sourceFile,
                               const QString &destination,
                               const QString &root,
                               const QString &skipPrefix)
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

        return false;
    }

    QDir destinationDir(destination);
    if (!root.isEmpty()) {
        destinationDir.setPath(destinationDir.filePath(root));
    }

    // Destination directory must be created before any other files.
    destinationDir.mkpath(QLatin1String("."));

    const QString destinationRoot = destinationDir.absolutePath();
    const QString destinationPrefix = destinationRoot.endsWith(QLatin1Char('/')) ? destinationRoot
                                                                                 : destinationRoot + QLatin1Char('/');

    // TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry = nullptr;
    while ((rc = archive_read_next_header(info.archiveHandle, &entry)) == ARCHIVE_OK) {
        // See https://github.com/libarchive/libarchive/issues/587 for more on UTF-8.
        QString pathname = QString::fromUtf8(archive_entry_pathname_utf8(entry));

        if (!root.isEmpty()) {
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        }

        // Unread entry data is skipped by the next header read.
        if (!skipPrefix.isEmpty() && (pathname == skipPrefix || pathname.startsWith(skipPrefix + QLatin1Char('/')))) {
            continue;
        }

        // Reject entries that resolve outside the destination (path traversal).
        const QString filePath = QDir::cleanPath(destinationDir.absoluteFilePath(pathname));
        if (filePath != destinationRoot && !filePath.startsWith(destinationPrefix)) {
            qCWarning(log, "Archive entry escapes destination: '%s'.", qPrintable(pathname));
            emit error(sourceFile, tr("Archive entry '%1' escapes the destination directory.").arg(pathname));
            archive_read_free(info.archiveHandle);
            return false;
        }

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

                return false;
            }

            const auto toWrite = static_cast<qint64>(size);
            const qint64 written = file.write(static_cast<const char *>(buffer), toWrite);
            if (written != toWrite) {
                qCWarning(log, "Cannot write extracted file data for '%s'.", qPrintable(pathname));
                emit error(sourceFile, tr("Failed to write extracted file '%1'.").arg(pathname));
                archive_read_free(info.archiveHandle);
                return false;
            }
        }

        emitProgress(info);
    }

    if (rc != ARCHIVE_EOF) {
        qCWarning(log, "Cannot read from archive: %s.", archive_error_string(info.archiveHandle));
        emit error(sourceFile, QString::fromLocal8Bit(archive_error_string(info.archiveHandle)));
        archive_read_free(info.archiveHandle);
        return false;
    }

    archive_read_free(info.archiveHandle);
    return true;
}

bool Extractor::extractTarixIndex(const QString &indexArchiveFile, const QString &indexPath)
{
    archive *handle = archive_read_new();
    archive_read_support_filter_all(handle);
    archive_read_support_format_all(handle);

    if (archive_read_open_filename(handle, qPrintable(indexArchiveFile), 10240) != ARCHIVE_OK) {
        qCWarning(log, "Cannot open tarix index archive: %s.", archive_error_string(handle));
        archive_read_free(handle);
        return false;
    }

    bool success = false;

    archive_entry *entry = nullptr;
    while (archive_read_next_header(handle, &entry) == ARCHIVE_OK) {
        if (archive_entry_filetype(entry) != S_IFREG) {
            continue;
        }

        QDir().mkpath(QFileInfo(indexPath).absolutePath());

        QFile file(indexPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(log, "Cannot open tarix index for writing at '%s'.", qPrintable(indexPath));
            break;
        }

        const void *buffer = nullptr;
        size_t size = 0;
        std::int64_t offset = 0;
        for (;;) {
            const int rc = archive_read_data_block(handle, &buffer, &size, &offset);
            if (rc == ARCHIVE_EOF) {
                success = true;
                break;
            }

            if (rc != ARCHIVE_OK
                || file.write(static_cast<const char *>(buffer), static_cast<qint64>(size))
                       != static_cast<qint64>(size)) {
                qCWarning(log, "Cannot extract tarix index: %s.", archive_error_string(handle));
                break;
            }
        }

        break; // The archive contains a single index file.
    }

    archive_read_free(handle);
    return success;
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
