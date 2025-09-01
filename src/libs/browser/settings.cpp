// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include "urlrequestinterceptor.h"

#include <core/settings.h>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace {
constexpr char HighlightOnNavigateCssPath[] = ":/browser/assets/css/highlight.css";
}

using namespace Zeal;
using namespace Zeal::Browser;

static Q_LOGGING_CATEGORY(log, "zeal.browser.settings")

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
    m_webProfile->settings()->setAttribute(QWebEngineSettings::ForceDarkMode,
                                           m_appSettings->isDarkModeEnabled());
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
    // Read the stylesheet file content.
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(log, "Failed to read stylesheet file '%s'.", qPrintable(path));
        return;
    }

    QString stylesheet = QString::fromUtf8(file.readAll());
    file.close();

    // Escape single quotes and newlines for JavaScript string literal.
    stylesheet.replace(QLatin1String("'"), QLatin1String("\\'"));
    stylesheet.replace(QLatin1String("\n"), QLatin1String("\\n"));

    QString cssInjectCode = QLatin1String(
        "(() => {"
            "const head = document.getElementsByTagName('head')[0];"
            "if (!head) { console.error('Cannot set custom stylesheet.'); return; }"
            "const stylesheet = document.createElement('style');"
            "stylesheet.setAttribute('type', 'text/css');"
            "stylesheet.textContent = '%1';"
            "head.appendChild(stylesheet);"
        "})()"
    );

    QWebEngineScript script;
    script.setName(name);
    script.setSourceCode(cssInjectCode.arg(stylesheet));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setRunsOnSubFrames(true);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    m_webProfile->scripts()->insert(script);
}
