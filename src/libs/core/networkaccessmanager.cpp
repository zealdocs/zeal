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

#include <QNetworkRequest>

using namespace Zeal::Core;

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                             const QNetworkRequest &request,
                                             QIODevice *outgoingData)
{
    // Detect URLs without schema, and prevent them from being requested on a local filesytem.
    const QUrl url = request.url();
    if (url.scheme() == QLatin1String("file") && !url.host().isEmpty())
        return QNetworkAccessManager::createRequest(GetOperation, QNetworkRequest(), outgoingData);

    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
