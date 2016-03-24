/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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

#include "networkaccessmanager.h"

#include <QNetworkRequest>

using namespace Zeal;

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                   const QNetworkRequest &req,
                                                   QIODevice *outgoingData)
{
    const QString scheme = req.url().scheme();

    if (scheme == QLatin1String("qrc"))
        return QNetworkAccessManager::createRequest(op, req, outgoingData);

    const bool nonFileScheme = scheme != QLatin1String("file");
    const bool nonLocalFile = !nonFileScheme && !req.url().host().isEmpty();

    // Ignore requests which cause Zeal to hang
    if (nonLocalFile || nonFileScheme) {
        return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation,
                                                    QNetworkRequest());
    }
#ifdef Q_OS_WIN32
    // Fix for AngularJS docset - Windows doesn't allow ':'s in filenames,
    // and bsdtar.exe replaces them with '_'s, so replace all ':'s in requests
    // with '_'s.
    QNetworkRequest winReq(req);
    QUrl winUrl(req.url());
    QString winPath = winUrl.path();
    // absolute paths are of form /C:/..., so don't replace colons occuring
    // within first 3 characters
    while (winPath.lastIndexOf(':') > 2)
        winPath = winPath.replace(winPath.lastIndexOf(':'), 1, "_");

    winUrl.setPath(winPath);
    winReq.setUrl(winUrl);
    return QNetworkAccessManager::createRequest(op, winReq, outgoingData);
#else
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
#endif
}
