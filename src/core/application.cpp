#include "application.h"

#include "extractor.h"
#include "settings.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QSettings>
#include <QThread>
#include <QUrlQuery>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
const char *LocalServerName = "ZealLocalServer";
}

Application *Application::m_instance = nullptr;

Application::Application(const QString &query, QObject *parent) :
    QObject(parent),
    m_settings(new Settings(this)),
    m_localServer(new QLocalServer(this)),
    m_networkManager(new QNetworkAccessManager(this)),
    m_extractorThread(new QThread(this)),
    m_extractor(new Extractor()),
    m_mainWindow(new MainWindow(this))
{
    // Ensure only one instance of Application
    Q_ASSERT(!m_instance);
    m_instance = this;

    // Server for detecting already running instances
    connect(m_localServer, &QLocalServer::newConnection, this, &Application::socketConnected);

    /// TODO: Verify if removeServer() is needed
    QLocalServer::removeServer(LocalServerName);  // remove in case previous instance crashed
    m_localServer->listen(LocalServerName);

    // Register for protocol queries
    associateProtocolHandler();

    // Extractor setup
    m_extractor->moveToThread(m_extractorThread);
    m_extractorThread->start();
    connect(m_extractor, &Extractor::completed, this, &Application::extractionCompleted);
    connect(m_extractor, &Extractor::error, this, &Application::extractionError);

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

void Application::extract(const QString &filePath, const QString &destination)
{
    QMetaObject::invokeMethod(m_extractor, "extract", Qt::QueuedConnection,
                              Q_ARG(QString, filePath), Q_ARG(QString, destination));
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

void Application::socketConnected() {
    QLocalSocket *connection = m_localServer->nextPendingConnection();
    // Wait a little while the other side writes the bytes
    connection->waitForReadyRead();
    if (connection->bytesAvailable()) {
        QueryType queryType;
        connection->read(reinterpret_cast<char*>(&queryType), sizeof(QueryType));
        QString query = QString::fromLocal8Bit(connection->readAll());

        switch (queryType) {
        case QueryType::DASH_PLUGIN:
            query = processPluginQuery(query);
            // fall through
        case QueryType::DASH:
            m_mainWindow->bringToFront(query);
            break;
        }
    }
}

QString Application::processPluginQuery(QString query)
{
    QUrl url(query);
    if (url.scheme() != QStringLiteral("dash-plugin")) {
        return QString();
    }

    size_t queryIndex = query.indexOf(QStringLiteral("dash-plugin://"));
    query.remove(0, queryIndex + 14);
    if (query.endsWith('/')) {
        query.remove(query.length() - 1, 1);
    }

    QUrlQuery parsedQuery(query);

    QString key = parsedQuery.queryItemValue(QStringLiteral("keys"));
    QString queryString = parsedQuery.queryItemValue(QStringLiteral("query"));

    if (key.isEmpty()) {
        return queryString;
    } else {
        return QString("%1:%2").arg(key, queryString);
    }
}

void Application::associateProtocolHandler()
{
#ifdef Q_OS_WIN32
    auto registerProtocol = [](const QString& protocol, const QString& description, const QString& command) {
        QSettings settings(QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(protocol), QSettings::NativeFormat);
        settings.setValue(QStringLiteral("Default"), description);
        settings.setValue(QStringLiteral("URL Protocol"), QString());

        QString path(QApplication::applicationFilePath());
        path.replace('/', '\\');
        settings.beginGroup(QStringLiteral("DefaultIcon"));
        settings.setValue(QStringLiteral("Default"), QString("%1,1").arg(path));
        settings.endGroup();

        settings.beginGroup(QStringLiteral("shell"));
        settings.beginGroup(QStringLiteral("open"));
        settings.beginGroup(QStringLiteral("command"));
        settings.setValue(QStringLiteral("Default"),
            QString("\"%1\" %2").arg(path, command));
    };
#endif

    registerProtocol(QStringLiteral("dash"),
        QStringLiteral("Zeal Search (Dash Protocol)"),
        QStringLiteral("--query \"%1\""));
    registerProtocol(QStringLiteral("dash-plugin"),
        QStringLiteral("Zeal Search (Dash Plugin Protocol)"),
        QStringLiteral("--plugin-query \"%1\""));
}
