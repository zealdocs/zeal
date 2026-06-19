// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webbridge.h"

#include <core/application.h>

#include <QDesktopServices>
#include <QSet>
#include <QUrl>

namespace Zeal::Browser {

namespace {
const QSet<QString> AllowedShortUrlKeys = {QStringLiteral("discord"),
                                           QStringLiteral("github"),
                                           QStringLiteral("report-bug"),
                                           QStringLiteral("telegram"),
                                           QStringLiteral("website"),
                                           QStringLiteral("x")};
} // namespace

WebBridge::WebBridge(QObject *parent)
    : QObject(parent)
{
}

void WebBridge::openShortUrl(const QString &key)
{
    if (!AllowedShortUrlKeys.contains(key)) {
        return;
    }

    QDesktopServices::openUrl(QUrl(QStringLiteral("https://go.zealdocs.org/l/") + key));
}

void WebBridge::triggerAction(const QString &action)
{
    emit actionTriggered(action);
}

QString WebBridge::appVersion()
{
    return Core::Application::versionString();
}

} // namespace Zeal::Browser
