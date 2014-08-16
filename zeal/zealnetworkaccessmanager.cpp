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
    const bool resourceFile = req.url().scheme() == "qrc";
    const bool nonLocalFile = req.url().scheme() == "file" && req.url().host() != "";
    const bool nonFile = req.url().scheme() != "file";
    if (resourceFile) {
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
    }
    if(nonLocalFile || nonFile) {
        // ignore requests which cause Zeal to hang
        return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation,
                                                    QNetworkRequest(QUrl()));
    }
#ifdef WIN32
    // Fix for AngularJS docset - Windows doesn't allow ':'s in filenames,
    // and bsdtar.exe replaces them with '_'s, so replace all ':'s in requests
    // with '_'s.
    QNetworkRequest winReq(req);
    QUrl winUrl(req.url());
    QString winPath = winUrl.path();
    // absolute paths are of form /C:/..., so don't replace colons occuring
    // within first 3 characters
    while(winPath.lastIndexOf(':') > 2) {
        winPath = winPath.replace(winPath.lastIndexOf(':'), 1, "_");
    }
    winUrl.setPath(winPath);
    winReq.setUrl(winUrl);
    return QNetworkAccessManager::createRequest(op, winReq, outgoingData);
#else
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
#endif
}
