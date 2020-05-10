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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "webpage.h"

#include <core/application.h>
#include <core/networkaccessmanager.h>
#include <core/settings.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

using namespace Zeal::Browser;

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(parent)
{
}

bool WebPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type)
    Q_UNUSED(isMainFrame)

    if (Core::NetworkAccessManager::isLocalUrl(url)) {
        return true;
    }

    switch (Core::Application::instance()->settings()->externalLinkPolicy) {
    case Core::Settings::ExternalLinkPolicy::Open:
        break;
    case Core::Settings::ExternalLinkPolicy::Ask: {
        QMessageBox mb;
        mb.setIcon(QMessageBox::Question);
        mb.setText(tr("How do you want to open the external link?<br>URL: <b>%1</b>")
                   .arg(url.toString()));


        QCheckBox *checkBox = new QCheckBox("Do &not ask again");
        mb.setCheckBox(checkBox);

        QPushButton *openInBrowserButton = mb.addButton(tr("Open in &Desktop Browser"), QMessageBox::ActionRole);
        QPushButton *openInZealButton = mb.addButton(tr("Open in &Zeal"), QMessageBox::ActionRole);
        mb.addButton(QMessageBox::Cancel);

        mb.setDefaultButton(openInBrowserButton);

        if (mb.exec() == QMessageBox::Cancel) {
            return false;
        }

        if (mb.clickedButton() == openInZealButton) {
            if (checkBox->isChecked()) {
                Core::Application::instance()->settings()->externalLinkPolicy
                        = Core::Settings::ExternalLinkPolicy::Open;
                Core::Application::instance()->settings()->save();
            }

            return true;
        }

        if (mb.clickedButton() == openInBrowserButton) {
            if (checkBox->isChecked()) {
                Core::Application::instance()->settings()->externalLinkPolicy
                        = Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser;
                Core::Application::instance()->settings()->save();
            }

            QDesktopServices::openUrl(url);
            return false;
        }

        break;
    }
    case Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser:
        QDesktopServices::openUrl(url);
        return false;
    }

    return false;
}
