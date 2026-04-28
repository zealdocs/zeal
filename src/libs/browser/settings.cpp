// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include "urlrequestinterceptor.h"

#include <core/settings.h>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>

namespace Zeal::Browser {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.browser.settings")

constexpr char HighlightOnNavigateCssPath[] = ":/browser/assets/css/highlight.css";
} // namespace

QWebEngineProfile *Settings::m_webProfile = nullptr;

Settings::Settings(Core::Settings *appSettings, QObject *parent)
    : QObject(parent)
    , m_appSettings(appSettings)
{
    Q_ASSERT(!m_webProfile);

    m_webProfile = QWebEngineProfile::defaultProfile();

    // Setup URL interceptor.
    m_webProfile->setUrlRequestInterceptor(new UrlRequestInterceptor(this));

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
    m_webProfile->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, m_appSettings->isDarkModeEnabled());
#endif

    // Apply custom CSS.
    // TODO: Apply to all open pages.
    m_webProfile->scripts()->clear(); // Remove all scripts first.

    if (m_appSettings->isHighlightOnNavigateEnabled) {
        setCustomStyleSheet(QStringLiteral("_zeal_highlightstylesheet"), HighlightOnNavigateCssPath);
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

void Settings::setCustomStyleSheet(const QString &name, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(log, "Failed to read stylesheet file '%s'.", qPrintable(path));
        return;
    }

    const QString b64 = QString::fromLatin1(file.readAll().toBase64());
    file.close();

    // clang-format off
    const QString cssInjectCode = QLatin1String(
        "(() => {"
            "const head = document.getElementsByTagName('head')[0];"
            "if (!head) { console.error('Cannot set custom stylesheet.'); return; }"
            "const bytes = Uint8Array.from(atob('%1'), c => c.charCodeAt(0));"
            "const stylesheet = document.createElement('style');"
            "stylesheet.setAttribute('type', 'text/css');"
            "stylesheet.textContent = new TextDecoder('utf-8').decode(bytes);"
            "head.appendChild(stylesheet);"
        "})()"
    );
    // clang-format on

    QWebEngineScript script;
    script.setName(name);
    script.setSourceCode(cssInjectCode.arg(b64));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setRunsOnSubFrames(true);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    m_webProfile->scripts()->insert(script);
}

} // namespace Zeal::Browser
