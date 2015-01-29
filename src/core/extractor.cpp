#include "extractor.h"

#include <QDir>

#include <archive.h>
#include <archive_entry.h>

using namespace Zeal::Core;

Extractor::Extractor(QObject *parent) :
    QObject(parent)
{
}

void Extractor::extract(const QString &filePath, const QString &destination)
{
    archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    int r = archive_read_open_filename(a, qPrintable(filePath), 10240);
    if (r) {
        emit error(filePath, QString::fromLocal8Bit(archive_error_string(a)));
        return;
    }

    const QDir destinationDir(destination);

    archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const QString pathname = destinationDir.absoluteFilePath(archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, qPrintable(pathname));
        archive_read_extract(a, entry, 0);
    }

    emit completed(filePath);
    archive_read_free(a);
}

