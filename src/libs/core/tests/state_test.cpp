// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../state.h"

#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using Zeal::Core::State;
using Zeal::Core::WindowState;

class StateTest : public QObject
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

void StateTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ZealTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ZealTest"));
}

void StateTest::loadMissingFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    State state;
    state.loadFromFile(dir.filePath("missing.toml"));

    QVERIFY(state.windows.isEmpty());
}

void StateTest::saveThenLoad_roundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("state.toml");

    const QByteArray geom = QByteArray::fromHex("0102030405deadbeef");
    const QByteArray splitter = QByteArray::fromHex("aabbccdd");
    const QByteArray tocSplitter = QByteArray::fromHex("cafebabe");

    {
        State out;
        WindowState ws;
        ws.geometry = geom;
        ws.splitterState = splitter;
        ws.tocSplitterState = tocSplitter;
        out.windows.append(ws);
        out.saveToFile(path);
    }

    State in;
    in.loadFromFile(path);
    QCOMPARE(in.windows.size(), 1);
    QCOMPARE(in.windows.first().geometry, geom);
    QCOMPARE(in.windows.first().splitterState, splitter);
    QCOMPARE(in.windows.first().tocSplitterState, tocSplitter);
}

void StateTest::loadCorruptFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("state.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not [valid toml");
    file.close();

    State state;
    state.loadFromFile(path);
    QVERIFY(state.windows.isEmpty());
}

void StateTest::loadEmptyFile_usesDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("state.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    State state;
    state.loadFromFile(path);
    QVERIFY(state.windows.isEmpty());
}

void StateTest::loadPartialFile_missingKeysUseDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("state.toml");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[[windows]]\ngeometry = \"AQIDBA==\"\n");
    file.close();

    State state;
    state.loadFromFile(path);
    QCOMPARE(state.windows.size(), 1);
    QCOMPARE(state.windows.first().geometry, QByteArray::fromHex("01020304"));
    QCOMPARE(state.windows.first().splitterState, QByteArray());
    QCOMPARE(state.windows.first().tocSplitterState, QByteArray());
}

void StateTest::load_migratesFromLegacyQSettings()
{
    const QByteArray geom = QByteArray::fromHex("deadbeef0102");
    const QByteArray splitter = QByteArray::fromHex("aabbccdd");
    const QByteArray tocSplitter = QByteArray::fromHex("11223344");

    // Populate legacy QSettings and make sure state.toml does not exist yet.
    {
        QSettings legacy;
        legacy.beginGroup(QStringLiteral("state"));
        legacy.setValue(QStringLiteral("window_geometry"), geom);
        legacy.setValue(QStringLiteral("splitter_geometry"), splitter);
        legacy.setValue(QStringLiteral("toc_splitter_state"), tocSplitter);
        legacy.endGroup();
        legacy.sync();
    }
    QFile::remove(State::defaultFilePath());

    State state;
    state.load();

    QCOMPARE(state.windows.size(), 1);
    QCOMPARE(state.windows.first().geometry, geom);
    QCOMPARE(state.windows.first().splitterState, splitter);
    QCOMPARE(state.windows.first().tocSplitterState, tocSplitter);

    // Legacy keys should be gone after migration.
    QSettings legacy;
    legacy.beginGroup(QStringLiteral("state"));
    QVERIFY(!legacy.contains(QStringLiteral("window_geometry")));
    QVERIFY(!legacy.contains(QStringLiteral("splitter_geometry")));
    QVERIFY(!legacy.contains(QStringLiteral("toc_splitter_state")));
}

void StateTest::load_skipsMigrationWhenStateFileExists()
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

    // Create state.toml with different values that SHOULD be loaded.
    {
        State out;
        WindowState ws;
        ws.geometry = tomlGeom;
        out.windows.append(ws);
        QVERIFY(out.saveToFile(State::defaultFilePath()));
    }

    State state;
    state.load();

    // Loaded from TOML, not legacy.
    QCOMPARE(state.windows.size(), 1);
    QCOMPARE(state.windows.first().geometry, tomlGeom);

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
    QFile::remove(State::defaultFilePath());
}

QTEST_GUILESS_MAIN(StateTest)
#include "state_test.moc"
