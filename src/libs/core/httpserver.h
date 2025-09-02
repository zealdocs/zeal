// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_HTTPSERVER_H
#define ZEAL_CORE_HTTPSERVER_H

#include <QObject>
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

    std::unique_ptr<httplib::Server> m_server;
    std::future<bool> m_future;

    QUrl m_baseUrl;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_HTTPSERVER_H
