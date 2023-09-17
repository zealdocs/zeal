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

#include <core/networkaccessmanager.h>

#include <QLoggingCategory>

using namespace Zeal::Browser;

static Q_LOGGING_CATEGORY(log, "zeal.browser.urlrequestinterceptor")

UrlRequestInterceptor::UrlRequestInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

void UrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const QUrl requestUrl = info.requestUrl();
    const QUrl firstPartyUrl = info.firstPartyUrl();

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
    // Workaround for https://github.com/zealdocs/zeal/issues/1376. Fixed in Qt 5.12.0.
    if (!firstPartyUrl.isValid() && requestUrl.scheme() == QLatin1String("blob")) {
        return;
    }
#endif

    // Block invalid requests.
    if (!requestUrl.isValid() || !firstPartyUrl.isValid()) {
        blockRequest(info);
        return;
    }

    // Direct links are controlled in the WebPage
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
        return;
    }

    bool isFirstPartyUrlLocal = Core::NetworkAccessManager::isLocalUrl(firstPartyUrl);
    bool isRequestUrlLocal = Core::NetworkAccessManager::isLocalUrl(requestUrl);

    // Allow local resources on local pages and external resources on external pages.
    if (isFirstPartyUrlLocal == isRequestUrlLocal) {
        return;
    }

    blockRequest(info);
}

void UrlRequestInterceptor::blockRequest(QWebEngineUrlRequestInfo &info)
{
    qCDebug(log, "Blocked request: %s '%s' (resource_type=%d, navigation_type=%d).",
            info.requestMethod().data(),
            qPrintable(info.requestUrl().toString()),
            info.resourceType(), info.navigationType());

    info.block(true);
}
