/****************************************************************************
**
** Copyright (C) 2020 Oleg Shparber
** Copyright (C) 2019 Kay Gawlik
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

#include "webpage.h"

#include "settings.h"

#include <core/application.h>
#include <core/networkaccessmanager.h>
#include <core/settings.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>

using namespace Zeal::Browser;

static Q_LOGGING_CATEGORY(log, "zeal.browser.webpage")

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(Settings::defaultProfile(), parent)
{
}

bool WebPage::acceptNavigationRequest(const QUrl &requestUrl, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type)

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

    auto appSettings = Core::Application::instance()->settings();

    // TODO: [C++20] using enum Core::Settings::ExternalLinkPolicy;
    typedef Core::Settings::ExternalLinkPolicy ExternalLinkPolicy;

    switch (appSettings->externalLinkPolicy) {
    case ExternalLinkPolicy::Open:
        return true;
    case ExternalLinkPolicy::Ask: {
        QMessageBox mb;
        mb.setIcon(QMessageBox::Question);
        mb.setText(tr("How do you want to open the external link?<br>URL: <b>%1</b>")
                   .arg(requestUrl.toString()));

        QCheckBox *checkBox = new QCheckBox("Do &not ask again");
        mb.setCheckBox(checkBox);

        QPushButton *openInBrowserButton = mb.addButton(tr("Open in &Desktop Browser"), QMessageBox::ActionRole);
        QPushButton *openInZealButton = mb.addButton(tr("Open in &Zeal"), QMessageBox::ActionRole);
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

            QDesktopServices::openUrl(requestUrl);
            return false;
        }

        break;
    }
    case ExternalLinkPolicy::OpenInSystemBrowser:
        QDesktopServices::openUrl(requestUrl);
        return false;
    }

    // This code should not be reachable so log a warning.
    qCWarning(log, "Blocked request to '%s'.", qPrintable(requestUrl.toString()));

    return false;
}

void WebPage::javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceId)
{
    Q_UNUSED(level)
    Q_UNUSED(message)
    Q_UNUSED(lineNumber)
    Q_UNUSED(sourceId)
}
