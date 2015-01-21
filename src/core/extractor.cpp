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

    // Save old working directory
    /// TODO: It's better to use archive_entry_set_pathname() instead of changing cwd
    const QString cwd = QDir::currentPath();
    QDir::setCurrent(destination);

    archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
        archive_read_extract(a, entry, 0);

    QDir::setCurrent(cwd);

    emit completed(filePath);
    archive_read_free(a);
}

