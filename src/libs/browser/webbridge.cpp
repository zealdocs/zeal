/****************************************************************************
**
** Copyright (C) 2018 Oleg Shparber
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

#include "webbridge.h"

#include <core/application.h>

#include <QDesktopServices>
#include <QUrl>

using namespace Zeal::Browser;

WebBridge::WebBridge(QObject *parent)
    : QObject(parent)
{
}

void WebBridge::openShortUrl(const QString &key)
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://go.zealdocs.org/l/") + key));
}

void WebBridge::triggerAction(const QString &action)
{
    emit actionTriggered(action);
}

QString WebBridge::appVersion() const
{
    return Core::Application::versionString();
}
