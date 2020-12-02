/****************************************************************************
**
** Copyright (C) 2020 Oleg Shparber
** Copyright (C) 2019 Kay Gawlik
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H
#define ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

namespace Zeal {
namespace Browser {

class UrlRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
    Q_DISABLE_COPY(UrlRequestInterceptor)
public:
    UrlRequestInterceptor(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    void blockRequest(QWebEngineUrlRequestInfo &info);
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H
