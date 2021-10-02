/****************************************************************************
**
** Copyright (C) 2016 Oleg Shparber
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

#include "networkaccessmanager.h"

#include "application.h"
#include "httpserver.h"

#include <QNetworkRequest>

using namespace Zeal::Core;

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

bool NetworkAccessManager::isLocalFile(const QUrl &url)
{
    return url.isLocalFile() || url.scheme() == QLatin1String("qrc");
}

bool NetworkAccessManager::isLocalUrl(const QUrl &url)
{
    if (isLocalFile(url)) {
        return true;
    }

    const QUrl &baseUrl = Application::instance()->httpServer()->baseUrl();
    if (baseUrl.isParentOf(url)) {
        return true;
    }

    return false;
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                   const QNetworkRequest &request,
                                                   QIODevice *outgoingData)
{
    QNetworkRequest overrideRequest(request);
    overrideRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    // Forward all non-local schemaless URLs via HTTPS.
    const QUrl url = request.url();
    if (isLocalFile(url) && !url.host().isEmpty()) {
        QUrl overrideUrl(url);
        overrideUrl.setScheme(QStringLiteral("https"));

        overrideRequest.setUrl(overrideUrl);

        op = QNetworkAccessManager::GetOperation;
    }

    return QNetworkAccessManager::createRequest(op, overrideRequest, outgoingData);
}
