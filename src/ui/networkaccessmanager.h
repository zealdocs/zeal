#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

namespace Zeal {

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit NetworkAccessManager(QObject *parent = nullptr);

    QNetworkReply *createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req,
                                 QIODevice *outgoingData = nullptr) override;
};

} // namespace Zeal

#endif // NETWORKACCESSMANAGER_H
