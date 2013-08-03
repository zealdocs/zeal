#include "zealnetworkaccessmanager.h"
#include <QDebug>
#include <QNetworkRequest>

ZealNetworkAccessManager::ZealNetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
}

QNetworkReply * ZealNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                        const QNetworkRequest& req,
                                                        QIODevice * outgoingData) {
    const bool nonLocalFile = req.url().scheme() == "file" && req.url().host() != "";
    const bool nonFile = req.url().scheme() != "file";
    if(nonLocalFile || nonFile) {
        // ignore requests which cause Zeal to hang
        return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation,
                                                    QNetworkRequest(QUrl()));
    }
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}
