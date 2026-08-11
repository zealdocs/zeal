// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../plist.h"

#include <QtTest>

using namespace Zeal::Util;

class PlistTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testReadsKeys();
    void testLeadingNewline();
    void testLeadingIndentation();
    void testByteOrderMark();
    void testMalformedContent();
    void testMissingFile();

private:
    QString writePlist(const QByteArray &data);

    QTemporaryDir m_dir;
    int m_counter = 0;
};

namespace {
const QByteArray Body = QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                          "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\""
                                          " \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                                          "<plist version=\"1.0\">\n"
                                          "  <dict>\n"
                                          "    <key>CFBundleName</key>\n"
                                          "    <string>Android KTX</string>\n"
                                          "    <key>isDashDocset</key>\n"
                                          "    <true/>\n"
                                          "    <key>isJavaScriptEnabled</key>\n"
                                          "    <false/>\n"
                                          "  </dict>\n"
                                          "</plist>\n");
} // namespace

QString PlistTest::writePlist(const QByteArray &data)
{
    const QString path = m_dir.filePath(QStringLiteral("Info%1.plist").arg(++m_counter));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
        return {};
    }

    return path;
}

void PlistTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

void PlistTest::testReadsKeys()
{
    Plist plist;
    QVERIFY(plist.read(writePlist(Body)));
    QVERIFY(!plist.hasError());
    QCOMPARE(plist.value(QStringLiteral("CFBundleName")), QVariant(QStringLiteral("Android KTX")));
    QCOMPARE(plist.value(QStringLiteral("isDashDocset")), QVariant(true));
    QCOMPARE(plist.value(QStringLiteral("isJavaScriptEnabled")), QVariant(false));
}

// See: https://github.com/zealdocs/zeal/issues/1949
void PlistTest::testLeadingNewline()
{
    Plist plist;
    QVERIFY(plist.read(writePlist(QByteArrayLiteral("\n") + Body)));
    QVERIFY(!plist.hasError());
    QCOMPARE(plist[QStringLiteral("CFBundleName")].toString(), QStringLiteral("Android KTX"));
}

void PlistTest::testLeadingIndentation()
{
    Plist plist;
    QVERIFY(plist.read(writePlist(QByteArrayLiteral("\n    ") + Body)));
    QVERIFY(!plist.hasError());
    QCOMPARE(plist[QStringLiteral("CFBundleName")].toString(), QStringLiteral("Android KTX"));
}

void PlistTest::testByteOrderMark()
{
    Plist plist;
    QVERIFY(plist.read(writePlist(QByteArrayLiteral("\xEF\xBB\xBF") + Body)));
    QVERIFY(!plist.hasError());
    QCOMPARE(plist[QStringLiteral("CFBundleName")].toString(), QStringLiteral("Android KTX"));
}

void PlistTest::testMalformedContent()
{
    const QString path = writePlist(QByteArrayLiteral("<plist><dict><key>CFBundleName</key>"));
    QVERIFY(!path.isEmpty());

    Plist plist;
    QVERIFY(!plist.read(path));
    QVERIFY(plist.hasError());
}

void PlistTest::testMissingFile()
{
    Plist plist;
    QVERIFY(!plist.read(m_dir.filePath(QStringLiteral("Missing.plist"))));
    QVERIFY(plist.hasError());
}

QTEST_GUILESS_MAIN(PlistTest)
#include "plist_test.moc"
