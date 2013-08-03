#ifndef ZEALNETWORKACCESSMANAGER_H
#define ZEALNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

class ZealNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit ZealNetworkAccessManager(QObject *parent = 0);
    QNetworkReply * createRequest(QNetworkAccessManager::Operation op,
                                  const QNetworkRequest& req,
                                  QIODevice * outgoingData = 0) override;
    
signals:
    
public slots:
    
};

#endif // ZEALNETWORKACCESSMANAGER_H
