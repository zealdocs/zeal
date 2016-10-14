/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

#ifdef USE_WEBENGINE
#include <QWebEngineSettings>
typedef QWebEngineSettings QWebSettings;
#else
#include <QWebSettings>
#endif

namespace {
// Configuration file groups
const char GroupBrowser[] = "browser";
const char GroupDocsets[] = "docsets";
const char GroupGlobalShortcuts[] = "global_shortcuts";
const char GroupTabs[] = "tabs";
const char GroupInternal[] = "internal";
const char GroupState[] = "state";
const char GroupProxy[] = "proxy";
}

using namespace Zeal::Core;

Settings::Settings(QObject *parent) :
    QObject(parent)
{
    // TODO: Move to user style sheet (related to #268)
#ifndef USE_WEBENGINE
    QWebSettings::globalSettings()
            ->setUserStyleSheetUrl(QUrl(QStringLiteral("qrc:///browser/highlight.css")));
#endif

    load();
}

Settings::~Settings()
{
    save();
}

void Settings::load()
{
    QScopedPointer<QSettings> settings(qsettings());
    migrate(settings.data());

    // TODO: Put everything in groups
    startMinimized = settings->value(QStringLiteral("start_minimized"), false).toBool();
    checkForUpdate = settings->value(QStringLiteral("check_for_update"), true).toBool();

    showSystrayIcon = settings->value(QStringLiteral("show_systray_icon"), true).toBool();
    minimizeToSystray = settings->value(QStringLiteral("minimize_to_systray"), false).toBool();
    hideOnClose = settings->value(QStringLiteral("hide_on_close"), false).toBool();

    settings->beginGroup(GroupGlobalShortcuts);
    showShortcut = settings->value(QStringLiteral("show")).value<QKeySequence>();
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    openNewTabAfterActive = settings->value(QStringLiteral("open_new_tab_after_active"), false).toBool();
    settings->endGroup();

    settings->beginGroup(GroupBrowser);
    minimumFontSize = settings->value(QStringLiteral("minimum_font_size"),
                                        QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
    QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, minimumFontSize);
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    proxyType = static_cast<ProxyType>(settings->value(QStringLiteral("type"),
                                                         ProxyType::System).toUInt());
    proxyHost = settings->value(QStringLiteral("host")).toString();
    proxyPort = static_cast<quint16>(settings->value(QStringLiteral("port"), 0).toUInt());
    proxyAuthenticate = settings->value(QStringLiteral("authenticate"), false).toBool();
    proxyUserName = settings->value(QStringLiteral("username")).toString();
    proxyPassword = settings->value(QStringLiteral("password")).toString();
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    if (settings->contains(QStringLiteral("path"))) {
        docsetPath = settings->value(QStringLiteral("path")).toString();
    } else {
#ifndef PORTABLE_BUILD
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + QLatin1String("/docsets");
#else
        docsetPath = QCoreApplication::applicationDirPath() + QLatin1String("/docsets");
#endif
        QDir().mkpath(docsetPath);
    }
    settings->endGroup();

    settings->beginGroup(GroupState);
    windowGeometry = settings->value(QStringLiteral("window_geometry")).toByteArray();
    verticalSplitterGeometry = settings->value(QStringLiteral("splitter_geometry")).toByteArray();
    tocSplitterState = settings->value(QStringLiteral("toc_splitter_state")).toByteArray();
    settings->endGroup();

    settings->beginGroup(GroupInternal);
    installId = settings->value(QStringLiteral("install_id"),
                                  // Avoid curly braces (QTBUG-885)
                                  QUuid::createUuid().toString().mid(1, 36)).toString();
    settings->endGroup();
}

void Settings::save()
{
    QScopedPointer<QSettings> settings(qsettings());

    // TODO: Put everything in groups
    settings->setValue(QStringLiteral("start_minimized"), startMinimized);
    settings->setValue(QStringLiteral("check_for_update"), checkForUpdate);

    settings->setValue(QStringLiteral("show_systray_icon"), showSystrayIcon);
    settings->setValue(QStringLiteral("minimize_to_systray"), minimizeToSystray);
    settings->setValue(QStringLiteral("hide_on_close"), hideOnClose);

    settings->beginGroup(GroupGlobalShortcuts);
    settings->setValue(QStringLiteral("show"), showShortcut);
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    settings->setValue(QStringLiteral("open_new_tab_after_active"), openNewTabAfterActive);
    settings->endGroup();

    settings->beginGroup(GroupBrowser);
    settings->setValue(QStringLiteral("minimum_font_size"), minimumFontSize);
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    settings->setValue(QStringLiteral("type"), proxyType);
    settings->setValue(QStringLiteral("host"), proxyHost);
    settings->setValue(QStringLiteral("port"), proxyPort);
    settings->setValue(QStringLiteral("authenticate"), proxyAuthenticate);
    settings->setValue(QStringLiteral("username"), proxyUserName);
    settings->setValue(QStringLiteral("password"), proxyPassword);
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    settings->setValue(QStringLiteral("path"), docsetPath);
    settings->endGroup();

    settings->beginGroup(GroupState);
    settings->setValue(QStringLiteral("window_geometry"), windowGeometry);
    settings->setValue(QStringLiteral("splitter_geometry"), verticalSplitterGeometry);
    settings->setValue(QStringLiteral("toc_splitter_state"), tocSplitterState);
    settings->endGroup();

    settings->beginGroup(GroupInternal);
    settings->setValue(QStringLiteral("install_id"), installId);
    // Version of configuration file format, should match Zeal version. Used for migration rules.
    settings->setValue(QStringLiteral("version"), QCoreApplication::applicationVersion());
    settings->endGroup();

    settings->sync();

    emit updated();
}

/*!
 * \internal
 * \brief Migrates settings from older application versions.
 * \param settings QSettings object to update.
 *
 * The settings migration process relies on 'internal/version' option, that was introduced in the
 * release 0.2.0, so a missing option indicates pre-0.2 release.
 */
void Settings::migrate(QSettings *settings) const
{
    settings->beginGroup(GroupInternal);
    const QString version = settings->value(QStringLiteral("version")).toString();
    settings->endGroup();

    //
    // Pre 0.3
    //

    // Unset 'state/splitter_geometry', because custom styles were removed.
    if (version.isEmpty() || version.startsWith(QLatin1String("0.2"))) {
        settings->beginGroup(GroupState);
        settings->remove(QStringLiteral("splitter_geometry"));
        settings->endGroup();
    }
}

/*!
 * \internal
 * \brief Returns an initialized QSettings object.
 * \param parent Optional parent object.
 * \return QSettings object.
 *
 * QSettings is initialized according to build options, e.g. standard vs portable.
 * Caller is responsible for deleting the returned object.
 */
QSettings *Settings::qsettings(QObject *parent)
{
#ifndef PORTABLE_BUILD
    return new QSettings(parent);
#else
    return new QSettings(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"),
                         QSettings::IniFormat, parent);
#endif
}
