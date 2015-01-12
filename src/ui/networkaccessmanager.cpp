#include "networkaccessmanager.h"

#include <QNetworkRequest>

using namespace Zeal;

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                   const QNetworkRequest &req,
                                                   QIODevice *outgoingData)
{
    const QString scheme = req.url().scheme();

    if (scheme == QLatin1String("qrc"))
        return QNetworkAccessManager::createRequest(op, req, outgoingData);

    const bool nonFileScheme = scheme != QLatin1String("file");
    const bool nonLocalFile = !nonFileScheme && !req.url().host().isEmpty();

    // Ignore requests which cause Zeal to hang
    if (nonLocalFile || nonFileScheme) {
        return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation,
                                                    QNetworkRequest());
    }
#ifdef Q_OS_WIN32
    // Fix for AngularJS docset - Windows doesn't allow ':'s in filenames,
    // and bsdtar.exe replaces them with '_'s, so replace all ':'s in requests
    // with '_'s.
    QNetworkRequest winReq(req);
    QUrl winUrl(req.url());
    QString winPath = winUrl.path();
    // absolute paths are of form /C:/..., so don't replace colons occuring
    // within first 3 characters
    while (winPath.lastIndexOf(':') > 2)
        winPath = winPath.replace(winPath.lastIndexOf(':'), 1, "_");

    winUrl.setPath(winPath);
    winReq.setUrl(winUrl);
    return QNetworkAccessManager::createRequest(op, winReq, outgoingData);
#else
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
#endif
}
