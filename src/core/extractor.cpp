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
        .extractor = this,
        .archiveHandle = archive_read_new(),
        .filePath = filePath,
        .totalBytes = QFileInfo(filePath).size(),
        .extractedBytes = 0
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
        destinationDir = destinationDir.absoluteFilePath(root);

    /// TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry;
    while (archive_read_next_header(info.archiveHandle, &entry) == ARCHIVE_OK) {
        QString pathname = archive_entry_pathname(entry);
        if (!root.isEmpty())
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        archive_entry_set_pathname(entry, qPrintable(destinationDir.absoluteFilePath(pathname)));
        archive_read_extract(info.archiveHandle, entry, 0);
    }

    emit completed(filePath);
    archive_read_free(info.archiveHandle);
}

void Extractor::progressCallback(void *ptr)
{
    ExtractInfo *info = reinterpret_cast<ExtractInfo *>(ptr);

    const qint64 extractedBytes = archive_filter_bytes(info->archiveHandle, -1);
    if (extractedBytes == info->extractedBytes)
        return;

    info->extractedBytes = extractedBytes;

    emit info->extractor->progress(info->filePath, extractedBytes, info->totalBytes);
}

