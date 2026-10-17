// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../docset.h"
#include "../searchresult.h"

#include <util/database.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <atomic>
#include <memory>

using Zeal::Registry::Docset;

class DocsetTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testPlainSearch_data();
    void testPlainSearch();

private:
    QTemporaryDir m_dir;
    std::unique_ptr<Docset> m_docset;
};

void DocsetTest::initTestCase()
{
    QVERIFY(m_dir.isValid());

    const QString docsetPath = m_dir.filePath(QStringLiteral("Test.docset"));
    QVERIFY(QDir().mkpath(docsetPath + QStringLiteral("/Contents/Resources/Documents")));

    QFile plist(docsetPath + QStringLiteral("/Contents/Info.plist"));
    QVERIFY(plist.open(QIODevice::WriteOnly));
    plist.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0"><dict><key>CFBundleName</key><string>Test</string></dict></plist>
)");
    plist.close();

    {
        Zeal::Util::Database db(docsetPath + QStringLiteral("/Contents/Resources/docSet.dsidx"));
        QVERIFY(db.isOpen());
        QVERIFY(db.execute(
            QStringLiteral("CREATE TABLE searchIndex(id INTEGER PRIMARY KEY, name TEXT, type TEXT, path TEXT)")));
        QVERIFY(db.execute(QStringLiteral("INSERT INTO searchIndex (name, type, path) VALUES"
                                          " ('FooBar', 'Class', 'a.html'),"
                                          " ('café', 'Function', 'b.html'),"
                                          " ('CAFÉ', 'Constant', 'c.html'),"
                                          " ('cafe', 'Function', 'd.html'),"
                                          " ('κόσμος', 'Function', 'e.html'),"
                                          " ('ΚΌΣΜΟΣ', 'Constant', 'f.html')")));
    }

    m_docset = std::make_unique<Docset>(docsetPath);
    QVERIFY(m_docset->isValid());
    m_docset->setFuzzySearchEnabled(false);
}

void DocsetTest::testPlainSearch_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("ascii") << QStringLiteral("foobar") << QStringList{QStringLiteral("FooBar")};
    QTest::newRow("latin-lower") << QStringLiteral("café")
                                 << QStringList{QStringLiteral("CAFÉ"), QStringLiteral("café")};
    QTest::newRow("latin-upper") << QStringLiteral("CAFÉ")
                                 << QStringList{QStringLiteral("CAFÉ"), QStringLiteral("café")};
    QTest::newRow("final-sigma") << QStringLiteral("κόσμος")
                                 << QStringList{QStringLiteral("ΚΌΣΜΟΣ"), QStringLiteral("κόσμος")};
    QTest::newRow("underscore-wildcard") << QStringLiteral("κ_σμος")
                                         << QStringList{QStringLiteral("ΚΌΣΜΟΣ"), QStringLiteral("κόσμος")};
    QTest::newRow("percent-wildcard") << QStringLiteral("c%É")
                                      << QStringList{QStringLiteral("CAFÉ"), QStringLiteral("café")};
    QTest::newRow("no-match") << QStringLiteral("κόσμοι") << QStringList{};
}

void DocsetTest::testPlainSearch()
{
    QFETCH(QString, query);
    QFETCH(QStringList, expected);

    const std::atomic_bool canceled{false};
    QStringList names;
    for (const auto &result : m_docset->search(query, canceled)) {
        names << result.name;
    }

    names.sort();
    QCOMPARE(names, expected);
}

QTEST_GUILESS_MAIN(DocsetTest)
#include "docset_test.moc"
