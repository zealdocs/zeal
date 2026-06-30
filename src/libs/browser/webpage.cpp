// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2019 Kay Gawlik
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webpage.h"

#include "settings.h"

#include <core/application.h>
#include <core/networkaccessmanager.h>
#include <core/settings.h>

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QWebEngineSettings>

namespace Zeal::Browser {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.browser.webpage")

void openInSystemBrowser(const QUrl &requestUrl)
{
    if (!QDesktopServices::openUrl(requestUrl)) {
        qCWarning(log, "Failed to open external URL in system browser: '%s'.", qPrintable(requestUrl.toString()));
    }
}
} // namespace

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(Settings::defaultProfile(), parent)
{
    // Set the page backdrop to match the theme, preventing a white flash during navigation.
    // TODO: Use a dedicated content appearance signal instead of the catch-all updated().
    applyBackgroundColor();
    connect(Core::Application::instance()->settings(), &Core::Settings::updated, this, &WebPage::applyBackgroundColor);

    // Tabs open on the qrc new-tab page, which themes itself; start with ForceDarkMode off
    // so its first render is not inverted. acceptNavigationRequest() sets it per navigated
    // URL, and a theme change re-applies it for the current page.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    settings()->setAttribute(QWebEngineSettings::ForceDarkMode, false);
#endif
    connect(Core::Application::instance()->settings(), &Core::Settings::updated, this, [this]() {
        applyForceDarkMode(url());
        updateInternalPageTheme();
    });

    // qrc fallback pages (e.g. not-found) can load without the theme fragment that browsertab
    // adds to the new-tab URL, so they start in the OS theme. Push Zeal's resolved theme once
    // the page is ready; updateInternalPageTheme() no-ops for non-qrc pages.
    connect(this, &QWebEnginePage::loadFinished, this, [this](bool ok) {
        if (ok) {
            updateInternalPageTheme();
        }
    });
}

void WebPage::applyBackgroundColor()
{
    setBackgroundColor(QApplication::palette().color(QPalette::Window));
}

void WebPage::applyForceDarkMode(const QUrl &pageUrl)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // qrc pages render their own theme (light, or hand-crafted dark via the data-theme
    // attribute); ForceDarkMode exists to darken docset content, so keep it off for them.
    const bool isInternal = pageUrl.scheme() == QStringLiteral("qrc");
    auto *appSettings = Core::Application::instance()->settings();
    settings()->setAttribute(QWebEngineSettings::ForceDarkMode, !isInternal && appSettings->isDarkModeEnabled());
#else
    Q_UNUSED(pageUrl)
#endif
}

void WebPage::updateInternalPageTheme()
{
    // qrc pages bake their theme in at load (URL fragment + inline script), which does not
    // react to a later theme change. Push the resolved theme to the open page so switching
    // appearance updates it live, mirroring what the inline script does on load.
    if (url().scheme() != QStringLiteral("qrc")) {
        return;
    }

    const QString theme = Core::Application::instance()->settings()->isDarkModeEnabled() ? QStringLiteral("dark")
                                                                                         : QStringLiteral("light");
    runJavaScript(QStringLiteral("var r = document.documentElement;"
                                 "r.setAttribute('data-theme', '%1');"
                                 "r.style.colorScheme = '%1';")
                      .arg(theme));
}

bool WebPage::acceptNavigationRequest(const QUrl &requestUrl, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type)

    // Keep ForceDarkMode (which darkens docset content) from inverting Zeal's own qrc
    // pages; they theme themselves via the data-theme set from the URL fragment.
    if (isMainFrame) {
        applyForceDarkMode(requestUrl);
    }

    // Local elements are always allowed.
    if (Core::NetworkAccessManager::isLocalUrl(requestUrl)) {
        return true;
    }

    // Allow external resources if already on an external page.
    const QUrl pageUrl = url();
    if (pageUrl.isValid() && !Core::NetworkAccessManager::isLocalUrl(pageUrl)) {
        return true;
    }

    // Block external elements on local pages.
    if (!isMainFrame) {
        qCDebug(log, "Blocked request to '%s'.", qPrintable(requestUrl.toString()));
        return false;
    }

    auto *appSettings = Core::Application::instance()->settings();

    using ExternalLinkPolicy = Core::Settings::ExternalLinkPolicy;

    switch (appSettings->externalLinkPolicy) {
    case ExternalLinkPolicy::Open:
        return true;
    case ExternalLinkPolicy::Ask: {
        // applyForceDarkMode() above ran against the external URL. If the user declines the
        // navigation, restore the current qrc page's theme so it is not left inverted.
        const bool accepted = promptForExternalLink(requestUrl);
        if (!accepted) {
            applyForceDarkMode(url());
        }
        return accepted;
    }
    case ExternalLinkPolicy::OpenInSystemBrowser:
        openInSystemBrowser(requestUrl);
        applyForceDarkMode(url());
        return false;
    }

    // This code should not be reachable so log a warning.
    qCWarning(log, "Blocked request to '%s'.", qPrintable(requestUrl.toString()));

    return false;
}

bool WebPage::promptForExternalLink(const QUrl &requestUrl)
{
    auto *appSettings = Core::Application::instance()->settings();

    using ExternalLinkPolicy = Core::Settings::ExternalLinkPolicy;

    QMessageBox mb;
    mb.setIcon(QMessageBox::Question);
    mb.setText(
        tr("How do you want to open the external link?<br>URL: <b>%1</b>").arg(requestUrl.toString().toHtmlEscaped()));

    auto *checkBox = new QCheckBox(tr("Do &not ask again"));
    mb.setCheckBox(checkBox);

    QPushButton *openInBrowserButton = mb.addButton(tr("Open in &Desktop Browser"), QMessageBox::ActionRole);
    const QPushButton *openInZealButton = mb.addButton(tr("Open in &Zeal"), QMessageBox::ActionRole);
    mb.addButton(QMessageBox::Cancel);

    mb.setDefaultButton(openInBrowserButton);

    if (mb.exec() == QMessageBox::Cancel) {
        qCDebug(log, "Blocked request to '%s'.", qPrintable(requestUrl.toString()));
        return false;
    }

    if (mb.clickedButton() == openInZealButton) {
        if (checkBox->isChecked()) {
            appSettings->externalLinkPolicy = ExternalLinkPolicy::Open;
            appSettings->save();
        }

        return true;
    }

    if (mb.clickedButton() == openInBrowserButton) {
        if (checkBox->isChecked()) {
            appSettings->externalLinkPolicy = ExternalLinkPolicy::OpenInSystemBrowser;
            appSettings->save();
        }

        openInSystemBrowser(requestUrl);
        return false;
    }

    // Dialog dismissed without a button click.
    qCWarning(log, "Blocked request to '%s'.", qPrintable(requestUrl.toString()));
    return false;
}

void WebPage::javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level,
                                       const QString &message,
                                       int lineNumber,
                                       const QString &sourceId)
{
    qCDebug(log,
            "%s [%s:%d] %s",
            qPrintable(QVariant::fromValue(level).toString()),
            qPrintable(sourceId),
            lineNumber,
            qPrintable(message));
}

} // namespace Zeal::Browser
