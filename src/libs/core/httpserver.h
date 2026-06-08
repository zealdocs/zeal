// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_HTTPSERVER_H
#define ZEAL_CORE_HTTPSERVER_H

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QReadWriteLock>
#include <QUrl>

#include <functional>
#include <future>
#include <memory>
#include <optional>

namespace httplib {
class Server;
} // namespace httplib

namespace Zeal::Core {

class HttpServer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HttpServer)
public:
    // Returns the file content for a mount-relative path, or nothing if absent.
    using ContentProvider = std::function<std::optional<QByteArray>(const QString &path)>;

    explicit HttpServer(QObject *parent = nullptr);
    ~HttpServer() override;

    QUrl baseUrl() const;

    QUrl mount(const QString &prefix, const QString &path);
    QUrl mount(const QString &prefix, ContentProvider provider);
    bool unmount(const QString &prefix);

private:
    QUrl prefixUrl(const QString &prefix) const;

    static QString sanitizePrefix(const QString &prefix);
    static QString resolvePathCaseInsensitive(const QString &root, const QString &path);

    std::unique_ptr<httplib::Server> m_server;

    QUrl m_baseUrl;
    QReadWriteLock m_mountPointsLock;
    QHash<QString, QString> m_mountPoints;
    QHash<QString, ContentProvider> m_contentProviders;

    // Must be last - its destructor blocks until the async thread completes,
    // ensuring the members above are still valid during shutdown.
    std::future<bool> m_future;
};

} // namespace Zeal::Core

#endif // ZEAL_CORE_HTTPSERVER_H
