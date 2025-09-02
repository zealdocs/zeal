// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_NETWORKACCESSMANAGER_H
#define ZEAL_CORE_NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

namespace Zeal {
namespace Core {

class NetworkAccessManager final : public QNetworkAccessManager
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(NetworkAccessManager)
public:
    NetworkAccessManager(QObject *parent = nullptr);

    static bool isLocalFile(const QUrl &url);
    static bool isLocalUrl(const QUrl &url);

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData = nullptr) override;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_NETWORKACCESSMANAGER_H
