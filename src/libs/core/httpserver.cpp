// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "httpserver.h"

#include "application.h"

#include <QDir>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QReadLocker>
#include <QRegularExpression>
#include <QWriteLocker>

#include <httplib.h>

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.httpserver")

constexpr const char *LocalHttpServerHost = "127.0.0.1"; // macOS only routes 127.0.0.1 by default.
} // namespace

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
{
    m_server = std::make_unique<httplib::Server>();

    const int port = m_server->bind_to_any_port(LocalHttpServerHost);

    m_baseUrl.setScheme(QStringLiteral("http"));
    m_baseUrl.setHost(QString::fromLatin1(LocalHttpServerHost));
    m_baseUrl.setPort(port);

    // NOLINTNEXTLINE(clang-analyzer-core.StackAddressEscape): false positive — cpp-httplib stores the handler by value.
    m_server->set_error_handler([this](const auto &req, auto &res) {
        // On 404, try case-insensitive path resolution.
        // Docsets generated on macOS (case-insensitive) may have links with mismatched case.
        // See: https://github.com/zealdocs/zeal/issues/1009
        if (res.status == 404) {
            const QString reqPath = QString::fromStdString(req.path);
            const QReadLocker locker(&m_mountPointsLock);
            // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage): Qt COW d-pointer confuses the analyzer.
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

    // Content-provider mounts share one catch-all route because cpp-httplib
    // cannot remove individual handlers. Directory mounts are served by
    // cpp-httplib before this handler is reached.
    // NOLINTNEXTLINE(clang-analyzer-core.StackAddressEscape): false positive — cpp-httplib stores the handler by value.
    m_server->Get("/.+", [this](const auto &req, auto &res) {
        const QString reqPath = QString::fromStdString(req.path);
        const qsizetype prefixEnd = reqPath.indexOf(QLatin1Char('/'), 1);
        const QString prefix = prefixEnd < 0 ? reqPath : reqPath.first(prefixEnd);
        QString path = prefixEnd < 0 ? QString() : reqPath.mid(prefixEnd + 1);

        // Hold the lock while reading so unmount cannot invalidate the provider mid-request.
        const QReadLocker locker(&m_mountPointsLock);
        const auto it = m_contentProviders.constFind(prefix);
        if (it == m_contentProviders.constEnd()) {
            res.status = 404;
            return;
        }

        if (path.isEmpty() || path.endsWith(QLatin1Char('/'))) {
            path += QLatin1String("index.html");
        }

        const std::optional<QByteArray> content = (*it)(path);
        if (!content) {
            res.status = 404;
            return;
        }

        // Extension-less docset pages are HTML.
        std::string mimeType = "text/html";
        const QString fileName = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
        if (fileName.contains(QLatin1Char('.'))) {
            mimeType = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchExtension).name().toStdString();
        }

        res.set_content(content->constData(), content->size(), mimeType);
    });

    m_future = std::async(std::launch::async, &httplib::Server::listen_after_bind, m_server.get());

    qCDebug(log, "Listening on %s...", qPrintable(m_baseUrl.toString()));
}

HttpServer::~HttpServer()
{
    m_server->stop();

    try {
        const auto status = m_future.wait_for(std::chrono::seconds(2));
        if (status != std::future_status::ready) {
            qCWarning(log) << "Failed to stop server within timeout.";
        }
    } catch (const std::future_error &e) {
        qCDebug(log, "Future has no associated state; nothing to wait for: %s.", e.what());
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
        return {};
    }

    {
        const QWriteLocker locker(&m_mountPointsLock);
        m_mountPoints[pfx] = path;
    }

    qCDebug(log, "Mounted '%s' to '%s'.", qPrintable(path), qPrintable(pfx));

    return prefixUrl(pfx);
}

QUrl HttpServer::mount(const QString &prefix, ContentProvider provider)
{
    const QString pfx = sanitizePrefix(prefix);

    {
        const QWriteLocker locker(&m_mountPointsLock);
        if (m_contentProviders.contains(pfx)) {
            qCWarning(log, "Prefix '%s' is already mounted.", qPrintable(pfx));
            return {};
        }
        m_contentProviders[pfx] = std::move(provider);
    }

    qCDebug(log, "Mounted content provider to '%s'.", qPrintable(pfx));

    return prefixUrl(pfx);
}

bool HttpServer::unmount(const QString &prefix)
{
    const QString pfx = sanitizePrefix(prefix);

    bool ok = false;
    {
        const QWriteLocker locker(&m_mountPointsLock);
        ok = m_contentProviders.remove(pfx);
    }

    if (!ok) {
        ok = m_server->remove_mount_point(pfx.toStdString());
        if (!ok) {
            qCWarning(log, "Failed to unmount '%s' to '%s'.", qPrintable(prefix), qPrintable(pfx));
        }

        const QWriteLocker locker(&m_mountPointsLock);
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

QUrl HttpServer::prefixUrl(const QString &prefix) const
{
    QUrl url = m_baseUrl;
    url.setPath(m_baseUrl.path() + prefix);
    return url;
}

QString HttpServer::sanitizePrefix(const QString &prefix)
{
    static const QRegularExpression invalidChars(QStringLiteral("[^a-zA-Z0-9-_]"));
    QString pfx = (prefix.startsWith(QLatin1String("/")) ? prefix.mid(1) : prefix).toLower();
    pfx.replace(invalidChars, QStringLiteral("_"));
    pfx.prepend(QLatin1Char('/'));

    return pfx;
}

} // namespace Zeal::Core
