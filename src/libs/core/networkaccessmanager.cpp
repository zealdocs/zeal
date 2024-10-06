// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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

    QSslConfiguration sslConfig = overrideRequest.sslConfiguration();
    sslConfig.setCaCertificates(QSslConfiguration::systemCaCertificates());
    overrideRequest.setSslConfiguration(sslConfig);

    return QNetworkAccessManager::createRequest(op, overrideRequest, outgoingData);
}
