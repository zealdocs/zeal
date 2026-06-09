// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_TARIXARCHIVE_H
#define ZEAL_UTIL_TARIXARCHIVE_H

#include <QByteArray>
#include <QMutex>
#include <QString>

#include <memory>
#include <optional>

namespace Zeal::Util {

class Database;

// Reads individual files from a tarix-compressed archive without unpacking it.
// The archive is a regular .tgz created with a zlib full flush point before
// every tar record, so decompression can start cold at any indexed offset.
// The index database maps archive paths to "<block> <offset> <length>"
// triples, where <offset> is the compressed byte offset of the flush point
// and <length> is the record size in 512-byte tar blocks.
class TarixArchive
{
    Q_DISABLE_COPY_MOVE(TarixArchive)
public:
    TarixArchive(const QString &archivePath, const QString &indexPath);
    ~TarixArchive();

    bool isOpen() const;
    QString lastError() const;

    // Reads one indexed document to confirm the index matches the archive.
    bool verify() const;

    // Paths are relative to the archive root directory,
    // e.g. "Contents/Resources/Documents/index.html". Lookups are case-insensitive.
    bool exists(const QString &path) const;
    std::optional<QByteArray> read(const QString &path) const;

private:
    QString lookupHash(const QString &storedPath) const;
    std::optional<QByteArray> readRecord(const QString &hash) const;

    QString m_archivePath;
    QString m_rootPrefix;
    QString m_lastError;

    mutable QMutex m_indexMutex;
    std::unique_ptr<Database> m_index;
};

} // namespace Zeal::Util

#endif // ZEAL_UTIL_TARIXARCHIVE_H
