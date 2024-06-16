/****************************************************************************
**
** Copyright (C) 2020 Oleg Shparber
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

#include "urlrequestinterceptor.h"

#include <core/settings.h>

#include <QFileInfo>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace {
constexpr char DarkModeCssUrl[] = "qrc:///browser/assets/css/darkmode.css";
constexpr char HighlightOnNavigateCssUrl[] = "qrc:///browser/assets/css/highlight.css";
}

using namespace Zeal;
using namespace Zeal::Browser;

QWebEngineProfile *Settings::m_webProfile = nullptr;

Settings::Settings(Core::Settings *appSettings, QObject *parent)
    : QObject(parent)
    , m_appSettings(appSettings)
{
    Q_ASSERT(!m_webProfile);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Create a new off-the-record profile.
    m_webProfile = new QWebEngineProfile(this);
#else
    // Default profile for Qt 6 is off-the-record.
    m_webProfile = QWebEngineProfile::defaultProfile();
#endif

    // Setup URL interceptor.
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    m_webProfile->setUrlRequestInterceptor(new UrlRequestInterceptor(this));
#else
    m_webProfile->setRequestInterceptor(new UrlRequestInterceptor(this));
#endif

    // Listen to settings changes.
    connect(m_appSettings, &Core::Settings::updated, this, &Settings::applySettings);
    applySettings();
}

void Settings::applySettings()
{
    m_webProfile->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,
                                           m_appSettings->isSmoothScrollingEnabled);

    // Qt 6.7+ does not require restart to enable dark mode.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_webProfile->settings()->setAttribute(QWebEngineSettings::ForceDarkMode,
                                           m_appSettings->isDarkModeEnabled());
#endif

    // Apply custom CSS.
    // TODO: Apply to all open pages.
    m_webProfile->scripts()->clear(); // Remove all scripts first.

    // Qt 5.14+ uses native Chromium dark mode.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    if (m_appSettings->isDarkModeEnabled()) {
        setCustomStyleSheet(QStringLiteral("_zeal_darkstylesheet"), DarkModeCssUrl);
    }
#endif

    if (m_appSettings->isHighlightOnNavigateEnabled) {
        setCustomStyleSheet(QStringLiteral("_zeal_highlightstylesheet"), HighlightOnNavigateCssUrl);
    }

    if (QFileInfo::exists(m_appSettings->customCssFile)) {
        setCustomStyleSheet(QStringLiteral("_zeal_userstylesheet"), m_appSettings->customCssFile);
    }
}

QWebEngineProfile *Settings::defaultProfile()
{
    Q_ASSERT(m_webProfile);
    return m_webProfile;
}

void Settings::setCustomStyleSheet(const QString &name, const QString &cssUrl)
{
    QString cssInjectCode = QLatin1String("(function() {"
                "let head = document.getElementsByTagName('head')[0];"
                "if (!head) { console.error('Cannot set custom stylesheet.'); return; }"
                "let link = document.createElement('link');"
                "link.rel = 'stylesheet';"
                "link.type = 'text/css';"
                "link.href = '%1';"
                "link.media = 'all';"
                "head.appendChild(link);"
                "})()");

    QWebEngineScript script;
    script.setName(name);
    script.setSourceCode(cssInjectCode.arg(cssUrl));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setRunsOnSubFrames(true);
    m_webProfile->scripts()->insert(script);
}
