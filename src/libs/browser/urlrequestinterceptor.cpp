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

#include "urlrequestinterceptor.h"

#include <core/application.h>
#include <core/networkaccessmanager.h>
#include <core/settings.h>

using namespace Zeal::Browser;

UrlRequestInterceptor::UrlRequestInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

void UrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const QUrl requestUrl = info.requestUrl();
    const QUrl firstPartyUrl = info.firstPartyUrl();

    if (!firstPartyUrl.isValid()) {
        return;
    }

    bool firstPartyUrlLocal = Core::NetworkAccessManager::isLocalUrl(firstPartyUrl);
    bool requestUrlLocal = Core::NetworkAccessManager::isLocalUrl(requestUrl);

    if (firstPartyUrlLocal && requestUrlLocal) {
        return;
    }

    if (firstPartyUrlLocal != requestUrlLocal) {
        blockRequest(info);
        return;
    }

    // TODO: [C++20] using enum Core::Settings::ExternalLinkPolicy;
    typedef Core::Settings::ExternalLinkPolicy ExternalLinkPolicy;

    ExternalLinkPolicy linkPolicy = Core::Application::instance()->settings()->externalLinkPolicy;
    switch (info.resourceType()) {
    case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
        if (linkPolicy != ExternalLinkPolicy::Open && linkPolicy != ExternalLinkPolicy::Ask) {
            blockRequest(info);
        }
        break;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
        if (linkPolicy != ExternalLinkPolicy::Open) {
            blockRequest(info);
        }
        break;
    default:
        break;
    }
}

void UrlRequestInterceptor::blockRequest(QWebEngineUrlRequestInfo &info)
{
    info.block(true);
}
