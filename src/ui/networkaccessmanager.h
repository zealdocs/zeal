#ifndef ZEALNETWORKACCESSMANAGER_H
#define ZEALNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

namespace Zeal {

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit NetworkAccessManager(QObject *parent = 0);
    QNetworkReply *createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req,
                                 QIODevice *outgoingData = 0) override;
};

} // namespace Zeal

#endif // ZEALNETWORKACCESSMANAGER_H
