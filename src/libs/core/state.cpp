// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "state.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>

#include <sstream>
#include <toml++/toml.hpp>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.state")

constexpr char ArrayWindows[] = "windows";
constexpr char KeyGeometry[] = "geometry";
constexpr char KeySplitter[] = "splitter";
constexpr char KeyTocSplitter[] = "toc_splitter";

QByteArray readBlob(const toml::table &tbl, const char *key)
{
    const auto node = tbl.get(key);
    if (node == nullptr || !node->is_string()) {
        return {};
    }

    const std::string &encoded = node->as_string()->get();
    return QByteArray::fromBase64(QByteArray::fromStdString(encoded));
}

void writeBlob(toml::table &tbl, const char *key, const QByteArray &blob)
{
    tbl.insert_or_assign(key, blob.toBase64().toStdString());
}
} // namespace

WindowState &State::primaryWindow()
{
    if (windows.isEmpty()) {
        windows.append(WindowState());
    }
    return windows.first();
}

QString State::defaultFilePath()
{
#ifndef PORTABLE_BUILD
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::StateLocation);
#else
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
#else
    const QString dir = QCoreApplication::applicationDirPath();
#endif
    return dir + QLatin1String("/state.toml");
}

void State::load()
{
    const QString path = defaultFilePath();
    if (QFile::exists(path)) {
        loadFromFile(path);
        return;
    }

    // First launch with the new state system: migrate the three blobs from the
    // legacy QSettings 'state' group. Persist state.toml before removing the
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

bool State::save() const
{
    return saveToFile(defaultFilePath());
}

void State::loadFromFile(const QString &path)
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
        qCWarning(log, "Failed to parse state file '%s': %s", qPrintable(path), err.description().data());
        return;
    }

    const auto windowsNode = root.get(ArrayWindows);
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

bool State::saveToFile(const QString &path) const
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

    const qint64 expected = static_cast<qint64>(text.size());
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
