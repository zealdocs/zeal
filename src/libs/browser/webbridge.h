// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_WEBBRIDGE_H
#define ZEAL_BROWSER_WEBBRIDGE_H

#include <QObject>

namespace Zeal {
namespace Browser {

class WebBridge final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebBridge)
    Q_PROPERTY(QString AppVersion READ appVersion CONSTANT)
public:
    explicit WebBridge(QObject *parent = nullptr);

signals:
    void actionTriggered(const QString &action);

public slots:
    Q_INVOKABLE void openShortUrl(const QString &key);
    Q_INVOKABLE void triggerAction(const QString &action);

private:
    QString appVersion() const;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_WEBBRIDGE_H
