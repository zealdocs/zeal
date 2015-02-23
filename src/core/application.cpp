#include "application.h"

#include "extractor.h"
#include "settings.h"
#include "registry/docsetregistry.h"
#include "registry/searchquery.h"
#include "ui/mainwindow.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QSysInfo>
#include <QThread>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
const char *LocalServerName = "ZealLocalServer";
}

Application *Application::m_instance = nullptr;

Application::Application(QObject *parent) :
    Application(SearchQuery(), parent)
{
}

Application::Application(const SearchQuery &query, QObject *parent) :
    QObject(parent)
{
    // Ensure only one instance of Application
    Q_ASSERT(!m_instance);
    m_instance = this;

    m_settings = new Settings(this);
    m_localServer = new QLocalServer(this);
    m_networkManager = new QNetworkAccessManager(this);
    m_extractorThread = new QThread(this);
    m_extractor = new Extractor();
    m_docsetRegistry = new DocsetRegistry();
    m_mainWindow = new MainWindow(this);

    // Server for detecting already running instances
    connect(m_localServer, &QLocalServer::newConnection, [this]() {
        QLocalSocket *connection = m_localServer->nextPendingConnection();
        // Wait a little while the other side writes the bytes
        connection->waitForReadyRead();
        if (connection->bytesAvailable()) {
            QDataStream in(connection);
            Zeal::SearchQuery query;
            in >> query;
            m_mainWindow->bringToFront(query);
        } else {
            m_mainWindow->bringToFront();
        }
    });
    /// TODO: Verify if removeServer() is needed
    QLocalServer::removeServer(LocalServerName);  // remove in case previous instance crashed
    m_localServer->listen(LocalServerName);

    // Extractor setup
    m_extractor->moveToThread(m_extractorThread);
    m_extractorThread->start();
    connect(m_extractor, &Extractor::completed, this, &Application::extractionCompleted);
    connect(m_extractor, &Extractor::error, this, &Application::extractionError);
    connect(m_extractor, &Extractor::progress, this, &Application::extractionProgress);

    connect(m_settings, &Settings::updated, this, &Application::applySettings);
    applySettings();

    if (!query.isEmpty())
        m_mainWindow->bringToFront(query);
    else if (!m_settings->startMinimized)
        m_mainWindow->show();
}

Application::~Application()
{
    m_extractorThread->quit();
    m_extractorThread->wait();
    delete m_extractor;
    delete m_mainWindow;
    delete m_docsetRegistry;
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

DocsetRegistry *Application::docsetRegistry()
{
    return m_instance->m_docsetRegistry;
}

void Application::extract(const QString &filePath, const QString &destination, const QString &root)
{
    QMetaObject::invokeMethod(m_extractor, "extract", Qt::QueuedConnection,
                              Q_ARG(QString, filePath), Q_ARG(QString, destination),
                              Q_ARG(QString, root));
}

QNetworkReply *Application::download(const QUrl &url)
{
    const static QString userAgent = QString("Zeal/%1 (%2 %3; Qt/%4)")
            .arg(QCoreApplication::applicationVersion())
#if QT_VERSION >= 0x050400
            .arg(QSysInfo::prettyProductName())
            .arg(QSysInfo::currentCpuArchitecture())
#else
            .arg("uknown-product")
            .arg("unknown-arch")
#endif
            .arg(qVersion());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
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
