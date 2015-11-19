/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
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
#define QWebSettings QWebEngineSettings
#else
#include <QWebSettings>
#endif

namespace {
// Configuration file groups
const char GroupBrowser[] = "browser";
const char GroupDocsets[] = "docsets";
const char GroupGlobalShortcuts[] = "global_shortcuts";
const char GroupInternal[] = "internal";
const char GroupState[] = "state";
const char GroupProxy[] = "proxy";
}

using namespace Zeal::Core;

Settings::Settings(QObject *parent) :
    QObject(parent),
    #ifndef PORTABLE_BUILD
    m_settings(new QSettings(this))
  #else
    m_settings(new QSettings(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"),
                             QSettings::IniFormat, this))
  #endif
{
    /// TODO: Move to user style sheet (related to #268)
#ifndef USE_WEBENGINE
    QWebSettings::globalSettings()->setUserStyleSheetUrl(QStringLiteral("qrc:///browser/highlight.css"));
#endif

    load();
}

Settings::~Settings()
{
    save();
}

void Settings::load()
{
    /// TODO: Put everything in groups
    startMinimized = m_settings->value(QStringLiteral("start_minimized"), false).toBool();
    checkForUpdate = m_settings->value(QStringLiteral("check_for_update"), true).toBool();

    showSystrayIcon = m_settings->value(QStringLiteral("show_systray_icon"), true).toBool();
    minimizeToSystray = m_settings->value(QStringLiteral("minimize_to_systray"), false).toBool();
    hideOnClose = m_settings->value(QStringLiteral("hide_on_close"), false).toBool();

    m_settings->beginGroup(GroupGlobalShortcuts);
#ifndef Q_OS_OSX
    showShortcut = m_settings->value(QStringLiteral("show"), QStringLiteral("Meta+Z")).value<QKeySequence>();
#else
    showShortcut = m_settings->value(QStringLiteral("show"), QStringLiteral("Alt+Space")).value<QKeySequence>();
#endif
    m_settings->endGroup();

    m_settings->beginGroup(GroupBrowser);
    minimumFontSize = m_settings->value(QStringLiteral("minimum_font_size"),
                                        QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
    m_settings->endGroup();

    m_settings->beginGroup(GroupProxy);
    proxyType = static_cast<ProxyType>(m_settings->value(QStringLiteral("type"),
                                                         ProxyType::System).toUInt());
    proxyHost = m_settings->value(QStringLiteral("host")).toString();
    proxyPort = m_settings->value(QStringLiteral("port"), 0).toInt();
    proxyAuthenticate = m_settings->value(QStringLiteral("authenticate"), false).toBool();
    proxyUserName = m_settings->value(QStringLiteral("username")).toString();
    proxyPassword = m_settings->value(QStringLiteral("password")).toString();
    m_settings->endGroup();

    m_settings->beginGroup(GroupDocsets);
    if (m_settings->contains(QStringLiteral("path"))) {
        docsetPath = m_settings->value(QStringLiteral("path")).toString();
    } else {
#ifndef PORTABLE_BUILD
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + QLatin1String("/docsets");
#else
        docsetPath = QCoreApplication::applicationDirPath() + QLatin1String("/docsets");
#endif
        QDir().mkpath(docsetPath);
    }
    m_settings->endGroup();

    m_settings->beginGroup(GroupState);
    windowGeometry = m_settings->value(QStringLiteral("window_geometry")).toByteArray();
    splitterGeometry = m_settings->value(QStringLiteral("splitter_geometry")).toByteArray();
    m_settings->endGroup();

    m_settings->beginGroup(GroupInternal);
    installId = m_settings->value(QStringLiteral("install_id"),
                                  // Avoid curly braces (QTBUG-885)
                                  QUuid::createUuid().toString().mid(1, 36)).toString();
    version = m_settings->value(QStringLiteral("version"),
                                QCoreApplication::applicationVersion()).toString();
    m_settings->endGroup();
}

void Settings::save()
{
    /// TODO: Put everything in groups
    m_settings->setValue(QStringLiteral("start_minimized"), startMinimized);
    m_settings->setValue(QStringLiteral("check_for_update"), checkForUpdate);

    m_settings->setValue(QStringLiteral("show_systray_icon"), showSystrayIcon);
    m_settings->setValue(QStringLiteral("minimize_to_systray"), minimizeToSystray);
    m_settings->setValue(QStringLiteral("hide_on_close"), hideOnClose);

    m_settings->beginGroup(GroupGlobalShortcuts);
    m_settings->setValue(QStringLiteral("show"), showShortcut);
    m_settings->endGroup();

    m_settings->beginGroup(GroupBrowser);
    m_settings->setValue(QStringLiteral("minimum_font_size"), minimumFontSize);
    m_settings->endGroup();

    m_settings->beginGroup(GroupProxy);
    m_settings->setValue(QStringLiteral("type"), proxyType);
    m_settings->setValue(QStringLiteral("host"), proxyHost);
    m_settings->setValue(QStringLiteral("port"), proxyPort);
    m_settings->setValue(QStringLiteral("authenticate"), proxyAuthenticate);
    m_settings->setValue(QStringLiteral("username"), proxyUserName);
    m_settings->setValue(QStringLiteral("password"), proxyPassword);
    m_settings->endGroup();

#ifndef PORTABLE_BUILD
    m_settings->beginGroup(GroupDocsets);
    m_settings->setValue(QStringLiteral("path"), docsetPath);
    m_settings->endGroup();
#endif

    m_settings->beginGroup(GroupState);
    m_settings->setValue(QStringLiteral("window_geometry"), windowGeometry);
    m_settings->setValue(QStringLiteral("splitter_geometry"), splitterGeometry);
    m_settings->endGroup();

    m_settings->beginGroup(GroupInternal);
    m_settings->setValue(QStringLiteral("install_id"), installId);
    m_settings->setValue(QStringLiteral("version"), QCoreApplication::applicationVersion());
    m_settings->endGroup();

    m_settings->sync();

    emit updated();
}
