// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "session.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>

#include <sstream>
#include <string_view>
#include <toml++/toml.hpp>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.session")

constexpr std::string_view ArrayWindows = "windows";
constexpr std::string_view KeyGeometry = "geometry";
constexpr std::string_view KeySplitter = "splitter";
constexpr std::string_view KeyTocSplitter = "toc_splitter";

QByteArray readBlob(const toml::table &tbl, std::string_view key)
{
    const auto *node = tbl.get(key);
    if (node == nullptr || !node->is_string()) {
        return {};
    }

    const std::string &encoded = node->as_string()->get();
    return QByteArray::fromBase64(QByteArray::fromStdString(encoded));
}

void writeBlob(toml::table &tbl, std::string_view key, const QByteArray &blob)
{
    tbl.insert_or_assign(key, blob.toBase64().toStdString());
}
} // namespace

WindowState &Session::primaryWindow()
{
    if (windows.isEmpty()) {
        windows.append(WindowState());
    }

    return windows.first();
}

QString Session::defaultFilePath()
{
#ifdef PORTABLE_BUILD
    return QCoreApplication::applicationDirPath() + QLatin1String("/session.toml");
#else
    // Test mode: isolate test runs under ~/.qttest/zeal/, mirroring how
    // QStandardPaths handles its own path categories in test mode.
    if (QStandardPaths::isTestModeEnabled()) {
        return QDir::homePath() + QLatin1String("/.qttest/zeal/session.toml");
    }

    // Compute the canonical platform path directly rather than via
    // QStandardPaths::StateLocation, which (a) is Qt 6.7+ only and (b) appends
    // "<organizationName>/<applicationName>" yielding the doubled "Zeal/Zeal"
    // path tracked in #1104. The path must not depend on Qt version — distro
    // Qt upgrades shouldn't relocate the state file.
#if defined(Q_OS_WIN)
    QString base = QString::fromLocal8Bit(qgetenv("LOCALAPPDATA"));
    if (base.isEmpty()) {
        base = QDir::homePath() + QLatin1String("/AppData/Local");
    }
    return base + QLatin1String("/Zeal/session.toml");
#elif defined(Q_OS_MACOS)
    return QDir::homePath() + QLatin1String("/Library/Application Support/Zeal/session.toml");
#else
    const QByteArray xdgState = qgetenv("XDG_STATE_HOME");
    const QString base = xdgState.isEmpty() ? QDir::homePath() + QLatin1String("/.local/state")
                                            : QString::fromLocal8Bit(xdgState);
    return base + QLatin1String("/zeal/session.toml");
#endif
#endif
}

void Session::load()
{
    const QString path = defaultFilePath();
    if (QFile::exists(path)) {
        loadFromFile(path);
        return;
    }

    // First launch with the new state system: migrate the three blobs from the
    // legacy QSettings 'state' group. Persist session.toml before removing the
    // legacy keys so a crash between the two steps doesn't lose user data —
    // migration retries on the next launch.
#ifndef PORTABLE_BUILD
    QSettings legacy;
#else
    QSettings legacy(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"), QSettings::IniFormat);
#endif
    legacy.beginGroup(QStringLiteral("state"));
    const QByteArray geometry = legacy.value(QStringLiteral("window_geometry")).toByteArray();
    const QByteArray splitter = legacy.value(QStringLiteral("splitter_geometry")).toByteArray();
    const QByteArray tocSplitter = legacy.value(QStringLiteral("toc_splitter_state")).toByteArray();
    legacy.endGroup();

    if (geometry.isEmpty() && splitter.isEmpty() && tocSplitter.isEmpty()) {
        return; // Nothing to migrate.
    }

    WindowState ws;
    ws.geometry = geometry;
    ws.splitterState = splitter;
    ws.tocSplitterState = tocSplitter;
    windows.append(ws);

    if (!saveToFile(path)) {
        return; // Leave legacy keys intact so migration retries next launch.
    }

    legacy.beginGroup(QStringLiteral("state"));
    legacy.remove(QStringLiteral("window_geometry"));
    legacy.remove(QStringLiteral("splitter_geometry"));
    legacy.remove(QStringLiteral("toc_splitter_state"));
    legacy.endGroup();
    legacy.sync();
}

bool Session::save() const
{
    return saveToFile(defaultFilePath());
}

void Session::loadFromFile(const QString &path)
{
    windows.clear();

    QFile file(path);
    if (!file.exists()) {
        return; // Defaults.
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(log, "Failed to open state file '%s': %s", qPrintable(path), qPrintable(file.errorString()));
        return;
    }

    const QByteArray contents = file.readAll();
    file.close();

    toml::table root;
    try {
        root = toml::parse(std::string_view(contents.constData(), static_cast<size_t>(contents.size())));
    } catch (const toml::parse_error &err) {
        qCWarning(log,
                  "Failed to parse state file '%s': %s",
                  qPrintable(path),
                  qPrintable(QString::fromUtf8(err.description())));
        return;
    }

    const auto *windowsNode = root.get(ArrayWindows);
    if (windowsNode == nullptr || !windowsNode->is_array()) {
        return;
    }

    for (const auto &node : *windowsNode->as_array()) {
        const auto *tbl = node.as_table();
        if (tbl == nullptr) {
            continue;
        }

        WindowState ws;
        ws.geometry = readBlob(*tbl, KeyGeometry);
        ws.splitterState = readBlob(*tbl, KeySplitter);
        ws.tocSplitterState = readBlob(*tbl, KeyTocSplitter);
        windows.append(ws);
    }
}

bool Session::saveToFile(const QString &path) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    toml::array windowsArray;
    for (const WindowState &ws : windows) {
        toml::table tbl;
        writeBlob(tbl, KeyGeometry, ws.geometry);
        writeBlob(tbl, KeySplitter, ws.splitterState);
        writeBlob(tbl, KeyTocSplitter, ws.tocSplitterState);
        windowsArray.push_back(std::move(tbl));
    }

    toml::table root;
    root.insert_or_assign(ArrayWindows, std::move(windowsArray));

    std::ostringstream oss;
    oss << "# Managed by Zeal. Do not edit manually.\n\n";
    oss << root;
    const std::string text = oss.str();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qCWarning(log, "Failed to write state file '%s': %s", qPrintable(path), qPrintable(file.errorString()));
        return false;
    }

    const auto expected = static_cast<qint64>(text.size());
    const qint64 written = file.write(text.c_str(), expected);
    file.close();

    if (written != expected) {
        qCWarning(log, "Partial write to state file '%s': %s", qPrintable(path), qPrintable(file.errorString()));
        // Don't leave a truncated/corrupt file behind — next launch should retry from scratch.
        QFile::remove(path);
        return false;
    }

    return true;
}

} // namespace Zeal::Core
