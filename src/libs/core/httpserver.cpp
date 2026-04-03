// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "httpserver.h"

#include "application.h"

#include <QDir>
#include <QLoggingCategory>
#include <QReadLocker>
#include <QRegularExpression>
#include <QWriteLocker>

#include <httplib.h>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.httpserver")

constexpr char LocalHttpServerHost[] = "127.0.0.1"; // macOS only routes 127.0.0.1 by default.
} // namespace

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
{
    m_server = std::make_unique<httplib::Server>();

    const int port = m_server->bind_to_any_port(LocalHttpServerHost);

    m_baseUrl.setScheme(QStringLiteral("http"));
    m_baseUrl.setHost(LocalHttpServerHost);
    m_baseUrl.setPort(port);

    m_server->set_error_handler([this](const auto &req, auto &res) {
        // On 404, try case-insensitive path resolution.
        // Docsets generated on macOS (case-insensitive) may have links with mismatched case.
        // See: https://github.com/zealdocs/zeal/issues/1009
        if (res.status == 404) {
            const QString reqPath = QString::fromStdString(req.path);
            QReadLocker locker(&m_mountPointsLock);
            for (auto it = m_mountPoints.constBegin(); it != m_mountPoints.constEnd(); ++it) {
                const QString prefix = it.key() + QLatin1Char('/');
                if (!reqPath.startsWith(prefix)) {
                    continue;
                }

                const QString relPath = reqPath.mid(prefix.length());
                const QString resolved = resolvePathCaseInsensitive(it.value(), relPath);
                if (!resolved.isEmpty()) {
                    const std::string redirect = QString(it.key() + resolved).toStdString();
                    res.set_redirect(redirect, 301);
                    return;
                }
            }
        }

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

    {
        QWriteLocker locker(&m_mountPointsLock);
        m_mountPoints[pfx] = path;
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

    {
        QWriteLocker locker(&m_mountPointsLock);
        m_mountPoints.remove(pfx);
    }

    qCDebug(log, "Unmounted prefix '%s' ('%s').", qPrintable(prefix), qPrintable(pfx));

    return ok;
}

// Walks the directory tree matching each path component case-insensitively.
// Returns the corrected relative path, or empty string if no match is found.
QString HttpServer::resolvePathCaseInsensitive(const QString &root, const QString &path)
{
    const QStringList components = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (components.isEmpty()) {
        return {};
    }

    QDir dir(root);
    QString resolved;

    for (const QString &component : components) {
        const QStringList entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);

        QString match;
        for (const QString &entry : entries) {
            if (entry.compare(component, Qt::CaseInsensitive) == 0) {
                match = entry;
                break;
            }
        }

        if (match.isEmpty()) {
            return {};
        }

        resolved += QLatin1Char('/') + match;
        dir.cd(match);
    }

    return resolved;
}

QString HttpServer::sanitizePrefix(const QString &prefix)
{
    QString pfx = (prefix.startsWith(QLatin1String("/")) ? prefix.mid(1) : prefix).toLower();
    pfx.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9-_]")), QStringLiteral("_"));
    pfx.prepend(QLatin1Char('/'));

    return pfx;
}

} // namespace Zeal::Core
