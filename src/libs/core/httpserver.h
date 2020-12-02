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
    Q_DISABLE_COPY(HttpServer)
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
