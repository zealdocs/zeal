// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../extractor.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <archive.h>
#include <archive_entry.h>

using Zeal::Core::Extractor;

namespace {
struct Entry
{
    QByteArray path;
    QByteArray content;
};

bool writeArchive(const QString &path, const QList<Entry> &entries, bool gzip = true)
{
    archive *a = archive_write_new();
    if (gzip) {
        archive_write_add_filter_gzip(a);
    }
    archive_write_set_format_ustar(a);

    bool ok = archive_write_open_filename(a, path.toLocal8Bit().constData()) == ARCHIVE_OK;
    for (const Entry &entry : entries) {
        if (!ok) {
            break;
        }

        archive_entry *e = archive_entry_new();
        archive_entry_set_pathname(e, entry.path.constData());
        archive_entry_set_size(e, entry.content.size());
        archive_entry_set_filetype(e, AE_IFREG);
        archive_entry_set_perm(e, 0644);
        ok = archive_write_header(a, e) == ARCHIVE_OK
          && archive_write_data(a, entry.content.constData(), entry.content.size()) >= 0;
        archive_entry_free(e);
    }

    ok = (archive_write_close(a) == ARCHIVE_OK) && ok;
    archive_write_free(a);
    return ok;
}
} // namespace

class ExtractorTest : public QObject
{
    Q_OBJECT

private slots:
    void testExtractsRegularEntries();
    void testRejectsPathTraversal();
    void testRejectsTruncatedArchive();
};

void ExtractorTest::testExtractsRegularEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString archivePath = dir.filePath(QStringLiteral("docset.tgz"));
    QVERIFY(writeArchive(archivePath,
                         {{"Test.docset/Contents/Info.plist", "plist"},
                          {"Test.docset/Contents/Resources/docSet.dsidx", "index"}}));

    Extractor extractor;
    QSignalSpy completedSpy(&extractor, &Extractor::completed);
    QSignalSpy errorSpy(&extractor, &Extractor::error);

    extractor.extract(archivePath, dir.path(), QStringLiteral("Test.docset"));

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("Test.docset/Contents/Info.plist"))));
}

void ExtractorTest::testRejectsPathTraversal()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString archivePath = dir.filePath(QStringLiteral("docset.tgz"));
    QVERIFY(writeArchive(archivePath, {{"Test.docset/../pwned.txt", "owned"}}));

    Extractor extractor;
    QSignalSpy completedSpy(&extractor, &Extractor::completed);
    QSignalSpy errorSpy(&extractor, &Extractor::error);

    extractor.extract(archivePath, dir.path(), QStringLiteral("Test.docset"));

    QCOMPARE(completedSpy.count(), 0);
    QVERIFY(errorSpy.count() > 0);
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("pwned.txt"))));
}

void ExtractorTest::testRejectsTruncatedArchive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Uncompressed, so the truncation lands on a known tar boundary.
    const QString archivePath = dir.filePath(QStringLiteral("docset.tar"));
    QVERIFY(writeArchive(archivePath,
                         {{"Test.docset/Contents/a.txt", QByteArray(512, 'a')},
                          {"Test.docset/Contents/b.txt", QByteArray(512, 'b')}},
                         false));

    // Cut into the middle of the second entry's 512-byte header.
    QFile file(archivePath);
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.resize(512 + 512 + 256));
    file.close();

    Extractor extractor;
    QSignalSpy completedSpy(&extractor, &Extractor::completed);
    QSignalSpy errorSpy(&extractor, &Extractor::error);

    extractor.extract(archivePath, dir.path(), QStringLiteral("Test.docset"));

    QCOMPARE(completedSpy.count(), 0);
    QVERIFY(errorSpy.count() > 0);
    // The first entry was written before the failure; the install is not reported complete.
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("Test.docset/Contents/a.txt"))));
}

QTEST_GUILESS_MAIN(ExtractorTest)
#include "extractor_test.moc"
