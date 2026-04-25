// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../session.h"

#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using Zeal::Core::Session;
using Zeal::Core::WindowState;

class SessionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadMissingFile_usesDefaults();
    void saveThenLoad_roundTrip();
    void loadCorruptFile_usesDefaults();
    void loadEmptyFile_usesDefaults();
    void loadPartialFile_missingKeysUseDefaults();
    void load_migratesFromLegacyQSettings();
    void load_skipsMigrationWhenStateFileExists();
};

void SessionTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ZealTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ZealTest"));
}

void SessionTest::loadMissingFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Session session;
    session.loadFromFile(dir.filePath("missing.toml"));

    QVERIFY(session.windows.isEmpty());
}

void SessionTest::saveThenLoad_roundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("session.toml");

    const QByteArray geom = QByteArray::fromHex("0102030405deadbeef");
    const QByteArray splitter = QByteArray::fromHex("aabbccdd");
    const QByteArray tocSplitter = QByteArray::fromHex("cafebabe");

    {
        Session out;
        WindowState ws;
        ws.geometry = geom;
        ws.splitterState = splitter;
        ws.tocSplitterState = tocSplitter;
        out.windows.append(ws);
        out.saveToFile(path);
    }

    Session in;
    in.loadFromFile(path);
    QCOMPARE(in.windows.size(), 1);
    QCOMPARE(in.windows.first().geometry, geom);
    QCOMPARE(in.windows.first().splitterState, splitter);
    QCOMPARE(in.windows.first().tocSplitterState, tocSplitter);
}

void SessionTest::loadCorruptFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("session.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not [valid toml");
    file.close();

    Session session;
    session.loadFromFile(path);
    QVERIFY(session.windows.isEmpty());
}

void SessionTest::loadEmptyFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("session.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    Session session;
    session.loadFromFile(path);
    QVERIFY(session.windows.isEmpty());
}

void SessionTest::loadPartialFile_missingKeysUseDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("session.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[[windows]]\ngeometry = \"AQIDBA==\"\n");
    file.close();

    Session session;
    session.loadFromFile(path);
    QCOMPARE(session.windows.size(), 1);
    QCOMPARE(session.windows.first().geometry, QByteArray::fromHex("01020304"));
    QCOMPARE(session.windows.first().splitterState, QByteArray());
    QCOMPARE(session.windows.first().tocSplitterState, QByteArray());
}

void SessionTest::load_migratesFromLegacyQSettings()
{
    const QByteArray geom = QByteArray::fromHex("deadbeef0102");
    const QByteArray splitter = QByteArray::fromHex("aabbccdd");
    const QByteArray tocSplitter = QByteArray::fromHex("11223344");

    // Populate legacy QSettings and make sure session.toml does not exist yet.
    {
        QSettings legacy;
        legacy.beginGroup(QStringLiteral("state"));
        legacy.setValue(QStringLiteral("window_geometry"), geom);
        legacy.setValue(QStringLiteral("splitter_geometry"), splitter);
        legacy.setValue(QStringLiteral("toc_splitter_state"), tocSplitter);
        legacy.endGroup();
        legacy.sync();
    }
    QFile::remove(Session::defaultFilePath());

    Session session;
    session.load();

    QCOMPARE(session.windows.size(), 1);
    QCOMPARE(session.windows.first().geometry, geom);
    QCOMPARE(session.windows.first().splitterState, splitter);
    QCOMPARE(session.windows.first().tocSplitterState, tocSplitter);

    // Legacy keys should be gone after migration.
    QSettings legacy;
    legacy.beginGroup(QStringLiteral("state"));
    QVERIFY(!legacy.contains(QStringLiteral("window_geometry")));
    QVERIFY(!legacy.contains(QStringLiteral("splitter_geometry")));
    QVERIFY(!legacy.contains(QStringLiteral("toc_splitter_state")));

    // Migration must persist the state so future runs load from TOML, not QSettings.
    QVERIFY(QFile::exists(Session::defaultFilePath()));

    // Cleanup so other tests start from a known state.
    QFile::remove(Session::defaultFilePath());
}

void SessionTest::load_skipsMigrationWhenStateFileExists()
{
    const QByteArray legacyGeom = QByteArray::fromHex("deadbeef");
    const QByteArray tomlGeom = QByteArray::fromHex("aabbccdd");

    // Populate legacy QSettings with values that should NOT be loaded.
    {
        QSettings legacy;
        legacy.beginGroup(QStringLiteral("state"));
        legacy.setValue(QStringLiteral("window_geometry"), legacyGeom);
        legacy.endGroup();
        legacy.sync();
    }

    // Create session.toml with different values that SHOULD be loaded.
    {
        Session out;
        WindowState ws;
        ws.geometry = tomlGeom;
        out.windows.append(ws);
        QVERIFY(out.saveToFile(Session::defaultFilePath()));
    }

    Session session;
    session.load();

    // Loaded from TOML, not legacy.
    QCOMPARE(session.windows.size(), 1);
    QCOMPARE(session.windows.first().geometry, tomlGeom);

    // Legacy keys must be untouched (migration was skipped).
    QSettings legacy;
    legacy.beginGroup(QStringLiteral("state"));
    QCOMPARE(legacy.value(QStringLiteral("window_geometry")).toByteArray(), legacyGeom);
    legacy.endGroup();

    // Cleanup so other tests start from a known state.
    {
        QSettings cleanup;
        cleanup.beginGroup(QStringLiteral("state"));
        cleanup.remove(QStringLiteral("window_geometry"));
        cleanup.endGroup();
    }
    QFile::remove(Session::defaultFilePath());
}

QTEST_GUILESS_MAIN(SessionTest)
#include "session_test.moc"
