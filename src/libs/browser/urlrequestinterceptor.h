// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2019 Kay Gawlik
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H
#define ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

namespace Zeal::Browser {

class UrlRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UrlRequestInterceptor)
public:
    explicit UrlRequestInterceptor(QObject *parent = nullptr);
    ~UrlRequestInterceptor() override = default;

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    void blockRequest(QWebEngineUrlRequestInfo &info);
};

} // namespace Zeal::Browser

#endif // ZEAL_BROWSER_URLREQUESTINTERCEPTOR_H
