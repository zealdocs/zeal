// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
