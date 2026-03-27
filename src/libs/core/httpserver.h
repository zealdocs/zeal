// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_HTTPSERVER_H
#define ZEAL_CORE_HTTPSERVER_H

#include <QHash>
#include <QObject>
#include <QReadWriteLock>
#include <QUrl>

#include <future>
#include <memory>

namespace httplib {
class Server;
} // namespace httplib

namespace Zeal {
namespace Core {

class HttpServer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HttpServer)
public:
    explicit HttpServer(QObject *parent = nullptr);
    ~HttpServer() override;

    QUrl baseUrl() const;

    QUrl mount(const QString &prefix, const QString &path);
    bool unmount(const QString &prefix);

private:
    static QString sanitizePrefix(const QString &prefix);
    static QString resolvePathCaseInsensitive(const QString &root, const QString &path);

    std::unique_ptr<httplib::Server> m_server;

    QUrl m_baseUrl;
    QReadWriteLock m_mountPointsLock;
    QHash<QString, QString> m_mountPoints;

    // Must be last - its destructor blocks until the async thread completes,
    // ensuring the members above are still valid during shutdown.
    std::future<bool> m_future;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_HTTPSERVER_H
