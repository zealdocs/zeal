// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../database.h"
#include "../statement.h"
#include "../tarixarchive.h"

#include <QTemporaryDir>
#include <QtTest>

#include <zlib.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Zeal::Util;

namespace {
constexpr qsizetype TarBlockSize = 512;

QByteArray tarHeader(const QByteArray &name, qint64 size, char typeflag)
{
    QByteArray header(TarBlockSize, '\0');
    char *h = header.data();

    std::memcpy(h, name.constData(), qMin<qsizetype>(name.size(), 100));
    std::snprintf(h + 100, 8, "%07o", 0644u); // mode
    std::snprintf(h + 108, 8, "%07o", 0u);    // uid
    std::snprintf(h + 116, 8, "%07o", 0u);    // gid
    std::snprintf(h + 124, 12, "%011llo", static_cast<unsigned long long>(size));
    std::snprintf(h + 136, 12, "%011llo", 0ULL); // mtime
    std::memset(h + 148, ' ', 8);                // chksum placeholder
    h[156] = typeflag;
    std::memcpy(h + 257, "ustar", 6); // magic
    std::memcpy(h + 263, "00", 2);    // version

    unsigned int checksum = 0;
    for (qsizetype i = 0; i < TarBlockSize; ++i) {
        checksum += static_cast<unsigned char>(header.at(i));
    }
    std::snprintf(h + 148, 8, "%06o", checksum);
    h[155] = ' ';

    return header;
}

QByteArray tarRecord(const QByteArray &name, const QByteArray &content)
{
    QByteArray record;

    // GNU long name record for paths exceeding the 100-byte header field.
    if (name.size() > 100) {
        const QByteArray nameData = name + '\0';
        record += tarHeader(QByteArrayLiteral("././@LongLink"), nameData.size(), 'L');
        record += nameData;
        record += QByteArray((TarBlockSize - nameData.size() % TarBlockSize) % TarBlockSize, '\0');
    }

    record += tarHeader(name.first(qMin<qsizetype>(name.size(), 100)), content.size(), '0');
    record += content;
    record += QByteArray((TarBlockSize - content.size() % TarBlockSize) % TarBlockSize, '\0');

    return record;
}

// Writes a gzip archive with a zlib full flush point before every record and
// returns "<block> <offset> <blockLength>" index hashes, mirroring tarix.
class TarixWriter
{
public:
    bool open(const QString &path)
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::WriteOnly)) {
            return false;
        }

        const int rc = deflateInit2(&m_zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK) {
            return false;
        }

        // Emit the gzip header and position the stream at a flush point.
        deflateChunk({}, Z_FULL_FLUSH);
        return true;
    }

    QString addRecord(const QByteArray &record)
    {
        const QString hash = QStringLiteral("%1 %2 %3")
                                 .arg(m_rawPos / TarBlockSize)
                                 .arg(m_zs.total_out)
                                 .arg(record.size() / TarBlockSize);
        deflateChunk(record, Z_FULL_FLUSH);
        m_rawPos += record.size();
        return hash;
    }

    void close()
    {
        deflateChunk(QByteArray(2 * TarBlockSize, '\0'), Z_FINISH);
        deflateEnd(&m_zs);
        m_file.close();
    }

private:
    void deflateChunk(const QByteArray &data, int flush)
    {
        // zlib operates on Bytef (unsigned char), so use matching buffers to
        // avoid char/unsigned-char punning.
        std::vector<Bytef> in(data.cbegin(), data.cend());
        m_zs.next_in = in.data();
        m_zs.avail_in = static_cast<uInt>(in.size());

        std::vector<Bytef> out(64 * 1024);
        int rc = Z_OK;
        do {
            m_zs.next_out = out.data();
            m_zs.avail_out = static_cast<uInt>(out.size());
            rc = deflate(&m_zs, flush);

            const qsizetype produced = static_cast<qsizetype>(out.size() - m_zs.avail_out);
            QByteArray chunk(produced, Qt::Uninitialized);
            std::copy_n(out.cbegin(), produced, chunk.begin());
            m_file.write(chunk);
        } while (m_zs.avail_out == 0 || m_zs.avail_in > 0 || (flush == Z_FINISH && rc != Z_STREAM_END));
    }

    QFile m_file;
    z_stream m_zs = {};
    qint64 m_rawPos = 0;
};
} // namespace

class TarixArchiveTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testMissingArchive();
    void testMissingIndex();
    void testIsOpen();
    void testExists();
    void testExistsCaseInsensitive();
    void testReadContent();
    void testReadEmptyFile();
    void testReadLongName();
    void testReadWindowsInvalidChars();
    void testReadMissing();
    void testReadRejectsBadHashFormats();
    void testVerify();
    void testVerifyWithoutDocuments();
    void testVerifyDetectsStaleIndex();

private:
    QTemporaryDir m_dir;
    QString m_archivePath;
    QString m_indexPath;
    QByteArray m_pageContent;
    QString m_longName;
};

void TarixArchiveTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_archivePath = m_dir.filePath(QStringLiteral("tarix.tgz"));
    m_indexPath = m_dir.filePath(QStringLiteral("tarixIndex.db"));

    // Compressible content spanning multiple tar blocks and inflate chunks.
    m_pageContent = QByteArrayLiteral("<html><body>");
    for (int i = 0; i < 20000; ++i) {
        m_pageContent += QByteArrayLiteral("<p>paragraph ") + QByteArray::number(i) + QByteArrayLiteral("</p>\n");
    }
    m_pageContent += QByteArrayLiteral("</body></html>");

    m_longName = QStringLiteral("Contents/Resources/Documents/") + QString(QLatin1Char('d')).repeated(120)
               + QStringLiteral("/page.html");

    TarixWriter writer;
    QVERIFY(writer.open(m_archivePath));

    const QByteArray root = QByteArrayLiteral("Test.docset/");
    QStringList hashes;
    hashes << writer.addRecord(tarRecord(root + "Contents/Resources/Documents/page.html", m_pageContent));
    hashes << writer.addRecord(tarRecord(root + "Contents/Resources/Documents/empty.txt", {}));
    hashes << writer.addRecord(tarRecord(root + m_longName.toUtf8(), QByteArrayLiteral("long name content")));
    hashes << writer.addRecord(
        tarRecord(root + "Contents/Resources/Documents/operator*.html", QByteArrayLiteral("star content")));
    writer.close();

    const QStringList paths = {QStringLiteral("Test.docset/Contents/Resources/Documents/page.html"),
                               QStringLiteral("Test.docset/Contents/Resources/Documents/empty.txt"),
                               QStringLiteral("Test.docset/") + m_longName,
                               QStringLiteral("Test.docset/Contents/Resources/Documents/operator*.html")};

    Database db(m_indexPath);
    QVERIFY(db.isOpen());
    QVERIFY(db.execute(QStringLiteral("CREATE TABLE tarindex(path TEXT PRIMARY KEY COLLATE NOCASE, hash TEXT)")));
    for (qsizetype i = 0; i < paths.size(); ++i) {
        Statement stmt(db, QStringLiteral("INSERT INTO tarindex VALUES(?, ?)"));
        stmt.bindText(1, paths.at(i));
        stmt.bindText(2, hashes.at(i));
        QVERIFY(!stmt.step());
        QVERIFY(stmt.lastError().isEmpty());
    }
}

void TarixArchiveTest::testMissingArchive()
{
    const TarixArchive archive(m_dir.filePath(QStringLiteral("nonexistent.tgz")), m_indexPath);
    QVERIFY(!archive.isOpen());
    QVERIFY(!archive.lastError().isEmpty());
}

void TarixArchiveTest::testMissingIndex()
{
    const TarixArchive archive(m_archivePath, m_dir.filePath(QStringLiteral("nonexistent.db")));
    QVERIFY(!archive.isOpen());
}

void TarixArchiveTest::testIsOpen()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    QVERIFY(archive.isOpen());
    QVERIFY(archive.lastError().isEmpty());
}

void TarixArchiveTest::testExists()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    QVERIFY(archive.exists(QStringLiteral("Contents/Resources/Documents/page.html")));
    QVERIFY(!archive.exists(QStringLiteral("Contents/Resources/Documents/missing.html")));
}

void TarixArchiveTest::testExistsCaseInsensitive()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    QVERIFY(archive.exists(QStringLiteral("contents/resources/documents/PAGE.HTML")));
}

void TarixArchiveTest::testReadContent()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    const auto content = archive.read(QStringLiteral("Contents/Resources/Documents/page.html"));
    QVERIFY(content.has_value());
    QCOMPARE(*content, m_pageContent);
}

void TarixArchiveTest::testReadEmptyFile()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    const auto content = archive.read(QStringLiteral("Contents/Resources/Documents/empty.txt"));
    QVERIFY(content.has_value());
    QVERIFY(content->isEmpty());
}

void TarixArchiveTest::testReadLongName()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    const auto content = archive.read(m_longName);
    QVERIFY(content.has_value());
    QCOMPARE(*content, QByteArrayLiteral("long name content"));
}

void TarixArchiveTest::testReadWindowsInvalidChars()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    const auto content = archive.read(QStringLiteral("Contents/Resources/Documents/operator*.html"));
    QVERIFY(content.has_value());
    QCOMPARE(*content, QByteArrayLiteral("star content"));
}

void TarixArchiveTest::testReadMissing()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    QVERIFY(!archive.read(QStringLiteral("Contents/Resources/Documents/missing.html")).has_value());
}

void TarixArchiveTest::testReadRejectsBadHashFormats()
{
    const QString badPath = m_dir.filePath(QStringLiteral("badhash.db"));
    const QStringList badHashes = {QStringLiteral("garbage"),
                                   QStringLiteral("0 5"),
                                   QStringLiteral("x y z"),
                                   QStringLiteral("0 -1 1"),
                                   QStringLiteral("0 0 0"),
                                   QStringLiteral("0 0 99999999")};
    {
        Database db(badPath);
        QVERIFY(db.isOpen());
        QVERIFY(db.execute(QStringLiteral("CREATE TABLE tarindex(path TEXT PRIMARY KEY COLLATE NOCASE, hash TEXT)")));
        for (qsizetype i = 0; i < badHashes.size(); ++i) {
            Statement stmt(db, QStringLiteral("INSERT INTO tarindex VALUES(?, ?)"));
            stmt.bindText(1, QStringLiteral("Test.docset/Contents/Resources/Documents/bad%1.html").arg(i));
            stmt.bindText(2, badHashes.at(i));
            QVERIFY(!stmt.step());
        }
    }

    const TarixArchive archive(m_archivePath, badPath);
    QVERIFY(archive.isOpen());
    for (qsizetype i = 0; i < badHashes.size(); ++i) {
        QVERIFY(!archive.read(QStringLiteral("Contents/Resources/Documents/bad%1.html").arg(i)).has_value());
    }
}

void TarixArchiveTest::testVerify()
{
    const TarixArchive archive(m_archivePath, m_indexPath);
    QVERIFY(archive.verify());
}

void TarixArchiveTest::testVerifyWithoutDocuments()
{
    const QString metaOnlyPath = m_dir.filePath(QStringLiteral("metaonly.db"));
    {
        Database db(metaOnlyPath);
        QVERIFY(db.isOpen());
        QVERIFY(db.execute(QStringLiteral("CREATE TABLE tarindex(path TEXT PRIMARY KEY COLLATE NOCASE, hash TEXT)")));
        Statement stmt(db, QStringLiteral("INSERT INTO tarindex VALUES(?, ?)"));
        stmt.bindText(1, QStringLiteral("Test.docset/Contents/Resources/docSet.dsidx"));
        stmt.bindText(2, QStringLiteral("0 0 1"));
        QVERIFY(!stmt.step());
    }

    const TarixArchive archive(m_archivePath, metaOnlyPath);
    QVERIFY(archive.isOpen());
    QVERIFY(!archive.verify());
}

void TarixArchiveTest::testVerifyDetectsStaleIndex()
{
    // An index whose offset does not point at a flush point in the archive.
    const QString stalePath = m_dir.filePath(QStringLiteral("stale.db"));
    {
        Database db(stalePath);
        QVERIFY(db.isOpen());
        QVERIFY(db.execute(QStringLiteral("CREATE TABLE tarindex(path TEXT PRIMARY KEY COLLATE NOCASE, hash TEXT)")));
        Statement stmt(db, QStringLiteral("INSERT INTO tarindex VALUES(?, ?)"));
        stmt.bindText(1, QStringLiteral("Test.docset/Contents/Resources/Documents/page.html"));
        stmt.bindText(2, QStringLiteral("0 5 1"));
        QVERIFY(!stmt.step());
    }

    const TarixArchive archive(m_archivePath, stalePath);
    QVERIFY(archive.isOpen());
    QVERIFY(!archive.verify());
}

QTEST_GUILESS_MAIN(TarixArchiveTest)
#include "tarixarchive_test.moc"
