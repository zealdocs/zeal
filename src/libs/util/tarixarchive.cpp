// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tarixarchive.h"

#include "database.h"
#include "statement.h"

#include <QFile>

#include <archive.h>
#include <archive_entry.h>
#include <zlib.h>

#include <bit>

namespace Zeal::Util {

namespace {
constexpr qint64 TarBlockSize = 512;
constexpr qsizetype InflateChunkSize = static_cast<qsizetype>(64) * 1024;
// Upper bound for a single record, guarding against corrupt index entries.
constexpr qint64 MaxRecordSize = static_cast<qint64>(64) * 1024 * 1024;

QByteArray inflateFrom(QFile &file, qint64 rawSize)
{
    z_stream zs = {};
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) {
        return {};
    }

    QByteArray out(rawSize, Qt::Uninitialized);
    QByteArray in(InflateChunkSize, Qt::Uninitialized);

    // next_out is set once; zlib advances it as it fills the output buffer.
    zs.next_out = std::bit_cast<Bytef *>(out.data());
    zs.avail_out = static_cast<uInt>(rawSize);

    int rc = Z_OK;
    while (zs.avail_out > 0 && rc != Z_STREAM_END) {
        const qint64 inSize = file.read(in.data(), in.size());
        if (inSize <= 0) {
            break;
        }

        zs.next_in = std::bit_cast<Bytef *>(in.data());
        zs.avail_in = static_cast<uInt>(inSize);

        while (zs.avail_in > 0 && zs.avail_out > 0) {
            rc = inflate(&zs, Z_NO_FLUSH);
            if (rc == Z_BUF_ERROR) {
                break; // Needs more input.
            }
            if (rc != Z_OK && rc != Z_STREAM_END) {
                inflateEnd(&zs);
                return {};
            }
            if (rc == Z_STREAM_END) {
                break;
            }
        }
    }

    inflateEnd(&zs);
    out.truncate(static_cast<qsizetype>(rawSize) - zs.avail_out);
    return out;
}

// libarchive transparently consumes any leading GNU long-name and PAX records,
// so the first returned header is the actual file.
std::optional<QByteArray> readTarEntry(const QByteArray &tar)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_tar(a);

    if (archive_read_open_memory(a, tar.constData(), tar.size()) != ARCHIVE_OK) {
        archive_read_free(a);
        return {};
    }

    std::optional<QByteArray> result;

    archive_entry *entry = nullptr;
    if (archive_read_next_header(a, &entry) == ARCHIVE_OK && archive_entry_filetype(entry) == AE_IFREG) {
        const auto size = static_cast<qint64>(archive_entry_size(entry));
        if (size >= 0 && size < tar.size()) {
            QByteArray content(size, Qt::Uninitialized);
            if (archive_read_data(a, content.data(), content.size()) == size) {
                result = std::move(content);
            }
        }
    }

    archive_read_free(a);
    return result;
}
} // namespace

TarixArchive::TarixArchive(const QString &archivePath, const QString &indexPath)
    : m_archivePath(archivePath)
{
    if (!QFile::exists(archivePath)) {
        m_lastError = QStringLiteral("Archive does not exist: %1").arg(archivePath);
        return;
    }

    // Check upfront because opening a nonexistent path would create an empty database.
    if (!QFile::exists(indexPath)) {
        m_lastError = QStringLiteral("Index does not exist: %1").arg(indexPath);
        return;
    }

    auto index = std::make_unique<Database>(indexPath);
    if (!index->isOpen()) {
        m_lastError = index->lastError();
        return;
    }

    if (!index->tables().contains(QStringLiteral("tarindex"), Qt::CaseInsensitive)) {
        m_lastError = QStringLiteral("Index has no 'tarindex' table: %1").arg(indexPath);
        return;
    }

    // Index paths start with the archive's root directory, e.g.
    // "Python.docset/Contents/...", while callers pass root-relative paths.
    Statement stmt(*index, QStringLiteral("SELECT path FROM tarindex WHERE path LIKE '%.docset/%' LIMIT 1"));
    if (stmt.step()) {
        const QString path = stmt.value(0).toString();
        const QLatin1String marker(".docset/");
        const qsizetype pos = path.indexOf(marker);
        if (pos >= 0) {
            m_rootPrefix = path.first(pos + marker.size());
        }
    }

    m_index = std::move(index);
}

TarixArchive::~TarixArchive() = default;

bool TarixArchive::isOpen() const
{
    return m_index != nullptr;
}

QString TarixArchive::lastError() const
{
    return m_lastError;
}

bool TarixArchive::verify() const
{
    if (m_index == nullptr) {
        return false;
    }

    QString storedPath;
    {
        const QMutexLocker locker(&m_indexMutex);
        Statement stmt(*m_index, QStringLiteral("SELECT path FROM tarindex WHERE path LIKE ? AND hash <> '' LIMIT 1"));
        stmt.bindText(1, QStringLiteral("%/Contents/Resources/Documents/%"));
        if (stmt.step()) {
            storedPath = stmt.value(0).toString();
        }
    }

    return !storedPath.isEmpty() && readRecord(lookupHash(storedPath)).has_value();
}

bool TarixArchive::exists(const QString &path) const
{
    return !lookupHash(m_rootPrefix + path).isEmpty();
}

std::optional<QByteArray> TarixArchive::read(const QString &path) const
{
    return readRecord(lookupHash(m_rootPrefix + path));
}

std::optional<QByteArray> TarixArchive::readRecord(const QString &hash) const
{
    const QList<QByteArray> fields = hash.toLatin1().split(' ');
    if (fields.size() != 3) {
        return {};
    }

    bool offsetOk = false;
    bool blockCountOk = false;
    const qint64 offset = fields.at(1).toLongLong(&offsetOk);
    const qint64 blockCount = fields.at(2).toLongLong(&blockCountOk);
    if (!offsetOk || !blockCountOk || offset < 0 || blockCount < 1 || blockCount > MaxRecordSize / TarBlockSize) {
        return {};
    }

    QFile archive(m_archivePath);
    if (!archive.open(QIODevice::ReadOnly) || !archive.seek(offset)) {
        return {};
    }

    const qint64 rawSize = blockCount * TarBlockSize;
    const QByteArray tar = inflateFrom(archive, rawSize);
    if (tar.size() != rawSize) {
        return {};
    }

    return readTarEntry(tar);
}

QString TarixArchive::lookupHash(const QString &storedPath) const
{
    if (m_index == nullptr) {
        return {};
    }

    const QMutexLocker locker(&m_indexMutex);

    Statement stmt(*m_index, QStringLiteral("SELECT hash FROM tarindex WHERE path = ?"));
    stmt.bindText(1, storedPath);
    if (!stmt.step()) {
        return {};
    }

    return stmt.value(0).toString();
}

} // namespace Zeal::Util
