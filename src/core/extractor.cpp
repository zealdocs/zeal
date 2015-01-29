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
    archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    int r = archive_read_open_filename(a, qPrintable(filePath), 10240);
    if (r) {
        emit error(filePath, QString::fromLocal8Bit(archive_error_string(a)));
        return;
    }

    QDir destinationDir(destination);
    if (!root.isEmpty())
        destinationDir = destinationDir.absoluteFilePath(root);

    // TODO: Do not strip root directory in archive if it equals to 'root'
    archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        QString pathname = archive_entry_pathname(entry);
        if (!root.isEmpty())
            pathname.remove(0, pathname.indexOf(QLatin1String("/")) + 1);
        archive_entry_set_pathname(entry, qPrintable(destinationDir.absoluteFilePath(pathname)));
        archive_read_extract(a, entry, 0);
    }

    emit completed(filePath);
    archive_read_free(a);
}

