#include "application.h"

#include "settings.h"
#include "ui/mainwindow.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
const char *LocalServerName = "ZealLocalServer";
}

Application::Application(const QString &query, QObject *parent) :
    QObject(parent),
    m_localServer(new QLocalServer(this)),
    m_settings(new Settings(this)),
    m_networkManager(new QNetworkAccessManager(this)),
    m_mainWindow(new MainWindow(m_settings))
{
    // Server for detecting already running instances
    connect(m_localServer, &QLocalServer::newConnection, [this]() {
        QLocalSocket *connection = m_localServer->nextPendingConnection();
        // Wait a little while the other side writes the bytes
        connection->waitForReadyRead();
        if (connection->bytesAvailable())
            m_mainWindow->bringToFrontAndSearch(QString::fromLocal8Bit(connection->readAll()));
    });
    /// TODO: Verify if removeServer() is needed
    QLocalServer::removeServer(LocalServerName);  // remove in case previous instance crashed
    m_localServer->listen(LocalServerName);

    connect(m_settings, &Settings::updated, this, &Application::applySettings);
    applySettings();

    if (!query.isEmpty())
        m_mainWindow->bringToFrontAndSearch(query);
    else if (!m_settings->startMinimized)
        m_mainWindow->show();
}

Application::~Application()
{
    delete m_mainWindow;
}

QString Application::localServerName()
{
    return LocalServerName;
}

QNetworkAccessManager *Application::networkManager() const
{
    return m_networkManager;
}

Settings *Application::settings() const
{
    return m_settings;
}

QNetworkReply *Application::download(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Zeal/" ZEAL_VERSION));
    return m_networkManager->get(request);
}

void Application::applySettings()
{
    // HTTP Proxy Settings
    switch (m_settings->proxyType) {
    case Core::Settings::ProxyType::None:
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;

    case Core::Settings::ProxyType::System:
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        break;

    case Core::Settings::ProxyType::UserDefined: {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, m_settings->proxyHost, m_settings->proxyPort);
        if (m_settings->proxyAuthenticate) {
            proxy.setUser(m_settings->proxyUserName);
            proxy.setPassword(m_settings->proxyPassword);
        }
        QNetworkProxy::setApplicationProxy(proxy);
        break;
    }
    }
}

