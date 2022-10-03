/****************************************************************************
**
** Copyright (C) 2020 Oleg Shparber
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

#include "httpserver.h"

#include "application.h"
#include "httplib.h"

#include <QLoggingCategory>
#include <QRegularExpression>

using namespace Zeal::Core;

namespace {
constexpr char LocalHttpServerHost[] = "127.0.0.1"; // macOS only routes 127.0.0.1 by default.
} // namespace

static Q_LOGGING_CATEGORY(log, "zeal.core.httpserver")

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
{
    m_server = std::make_unique<httplib::Server>();

    const int port = m_server->bind_to_any_port(LocalHttpServerHost);

    m_baseUrl.setScheme(QStringLiteral("http"));
    m_baseUrl.setHost(LocalHttpServerHost);
    m_baseUrl.setPort(port);

    m_server->set_error_handler([](const auto& req, auto& res) {
        const QString html = QStringLiteral("<b>ERROR %1</b><br><pre>Request path: %2</pre>")
                .arg(res.status)
                .arg(QString::fromStdString(req.path));
        res.set_content(html.toUtf8().data(), "text/html");
    });

    m_future = std::async(std::launch::async, &httplib::Server::listen_after_bind, m_server.get());

    qCDebug(log, "Listening on %s...", qPrintable(m_baseUrl.toString()));
}

HttpServer::~HttpServer()
{
    m_server->stop();

    auto status = m_future.wait_for(std::chrono::seconds(2));
    if (status != std::future_status::ready) {
        qCWarning(log) << "Failed to stop server within timeout.";
    }
}

QUrl HttpServer::baseUrl() const
{
    return m_baseUrl;
}

QUrl HttpServer::mount(const QString &prefix, const QString &path)
{
    const QString pfx = sanitizePrefix(prefix);
    const bool ok = m_server->set_mount_point(pfx.toStdString(), path.toStdString());
    if (!ok) {
        qCWarning(log, "Failed to mount '%s' to '%s'.", qPrintable(path), qPrintable(pfx));
        return QUrl();
    }

    qCDebug(log, "Mounted '%s' to '%s'.", qPrintable(path), qPrintable(pfx));

    QUrl mountUrl = m_baseUrl;
    mountUrl.setPath(m_baseUrl.path() + pfx);
    return mountUrl;
}

bool HttpServer::unmount(const QString &prefix)
{
    const QString pfx = sanitizePrefix(prefix);
    const bool ok = m_server->remove_mount_point(pfx.toStdString());
    if (!ok) {
        qCWarning(log, "Failed to unmount '%s' to '%s'.", qPrintable(prefix), qPrintable(pfx));
    }

    qCDebug(log, "Unmounted prefix '%s' ('%s').", qPrintable(prefix), qPrintable(pfx));

    return ok;
}

QString HttpServer::sanitizePrefix(const QString &prefix)
{
    QString pfx = (prefix.startsWith(QLatin1String("/")) ? prefix.right(1) : prefix).toLower();
    pfx.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9-_]")), QStringLiteral("_"));
    pfx.prepend(QLatin1Char('/'));

    return pfx;
}
