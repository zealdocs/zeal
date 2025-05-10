// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"

#include "extractor.h"
#include "filemanager.h"
#include "httpserver.h"
#include "networkaccessmanager.h"
#include "settings.h"

#include <registry/docsetregistry.h>
#include <ui/mainwindow.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QString>
#include <QSysInfo>
#include <QThread>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
constexpr char ReleasesApiUrl[] = "https://api.zealdocs.org/v1/releases";
} // namespace

Application *Application::m_instance = nullptr;

Application::Application(QObject *parent)
    : QObject(parent)
{
    // Ensure only one instance of Application
    Q_ASSERT(!m_instance);
    m_instance = this;

    m_settings = new Settings(this);
    m_networkManager = new NetworkAccessManager(this);

    m_fileManager = new FileManager(this);
    m_httpServer = new HttpServer(this);

    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            this, [this](QNetworkReply *reply, const QList<QSslError> &errors) {
        if (m_settings->isIgnoreSslErrorsEnabled) {
            reply->ignoreSslErrors();
        }
    });

    // Extractor setup
    m_extractorThread = new QThread(this);
    m_extractor = new Extractor();
    m_extractor->moveToThread(m_extractorThread);
    m_extractorThread->start();
    connect(m_extractor, &Extractor::completed, this, &Application::extractionCompleted);
    connect(m_extractor, &Extractor::error, this, &Application::extractionError);
    connect(m_extractor, &Extractor::progress, this, &Application::extractionProgress);

    m_docsetRegistry = new Registry::DocsetRegistry();

    connect(m_settings, &Settings::updated, this, &Application::applySettings);
    applySettings();

    m_mainWindow = new WidgetUi::MainWindow(this);

    if (m_settings->startMinimized) {
        if (m_settings->showSystrayIcon && m_settings->minimizeToSystray)
            return;

        m_mainWindow->showMinimized();
    } else {
        m_mainWindow->show();
    }
}

Application::~Application()
{
    m_extractorThread->quit();
    m_extractorThread->wait();
    delete m_extractor;
    delete m_mainWindow;
    delete m_docsetRegistry;
}

/*!
 * \internal
 * \brief Returns a pointer to the Core::Application instance.
 * \return A pointer or \c nullptr, if no instance has been created.
 */
Application *Application::instance()
{
    return m_instance;
}

WidgetUi::MainWindow *Application::mainWindow() const
{
    return m_mainWindow;
}

QNetworkAccessManager *Application::networkManager() const
{
    return m_networkManager;
}

Settings *Application::settings() const
{
    return m_settings;
}

Registry::DocsetRegistry *Application::docsetRegistry()
{
    return m_docsetRegistry;
}

FileManager *Application::fileManager() const
{
    return m_fileManager;
}

HttpServer *Application::httpServer() const
{
    return m_httpServer;
}

QString Application::cacheLocation()
{
#ifndef PORTABLE_BUILD
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    return QCoreApplication::applicationDirPath() + QLatin1String("/cache");
#endif
}

QString Application::configLocation()
{
#ifndef PORTABLE_BUILD
    // TODO: Replace 'Zeal/Zeal' with 'zeal'.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
    return QCoreApplication::applicationDirPath() + QLatin1String("/config");
#endif
}

QVersionNumber Application::version()
{
    static const auto vn = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    return vn;
}

QString Application::versionString()
{
    static const auto v = QStringLiteral("v%1").arg(QCoreApplication::applicationVersion());
    return v;
}

void Application::executeQuery(const QString &query, bool preventActivation)
{
    m_mainWindow->search(query);

    if (preventActivation)
        return;

    m_mainWindow->bringToFront();
}

void Application::extract(const QString &filePath, const QString &destination, const QString &root)
{
    QMetaObject::invokeMethod(m_extractor, "extract", Qt::QueuedConnection,
                              Q_ARG(QString, filePath), Q_ARG(QString, destination),
                              Q_ARG(QString, root));
}

QNetworkReply *Application::download(const QUrl &url)
{
    static const QString ua = userAgent();
    static const QByteArray uaJson = userAgentJson().toUtf8();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, ua);

    if (url.host().endsWith(QLatin1String(".zealdocs.org", Qt::CaseInsensitive)))
        request.setRawHeader("X-Zeal-User-Agent", uaJson);

    return m_networkManager->get(request);
}

/*!
  \internal

  Performs a check whether a new Zeal version is available. Setting \a quiet to true suppresses
  error and "you are using the latest version" message boxes.
*/
void Application::checkForUpdates(bool quiet)
{
    QNetworkReply *reply = download(QUrl(ReleasesApiUrl));
    connect(reply, &QNetworkReply::finished, this, [this, quiet]() {
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(
                    qobject_cast<QNetworkReply *>(sender()));

        if (reply->error() != QNetworkReply::NoError) {
            if (!quiet)
                emit updateCheckError(reply->errorString());
            return;
        }

        QJsonParseError jsonError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            if (!quiet)
                emit updateCheckError(jsonError.errorString());
            return;
        }

        const QJsonObject versionInfo = jsonDoc.array().first().toObject(); // Latest is the first.
        const auto latestVersion
                = QVersionNumber::fromString(versionInfo[QLatin1String("version")].toString());
        if (latestVersion > version()) {
            emit updateCheckDone(latestVersion.toString());
        } else if (!quiet) {
            emit updateCheckDone();
        }
    });
}

void Application::applySettings()
{
    m_docsetRegistry->setStoragePath(m_settings->docsetPath);
    m_docsetRegistry->setFuzzySearchEnabled(m_settings->isFuzzySearchEnabled);

    // HTTP Proxy Settings
    switch (m_settings->proxyType) {
    case Settings::ProxyType::None:
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;

    case Settings::ProxyType::System:
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        break;

    case Settings::ProxyType::Http:
    case Settings::ProxyType::Socks5: {
        const QNetworkProxy::ProxyType type = m_settings->proxyType == Settings::ProxyType::Socks5
                ? QNetworkProxy::Socks5Proxy
                : QNetworkProxy::HttpProxy;

        QNetworkProxy proxy(type, m_settings->proxyHost, m_settings->proxyPort);
        if (m_settings->proxyAuthenticate) {
            proxy.setUser(m_settings->proxyUserName);
            proxy.setPassword(m_settings->proxyPassword);
        }

        QNetworkProxy::setApplicationProxy(proxy);

        break;
    }
    }

    // Force NM to pick up changes.
    m_networkManager->clearAccessCache();
}

QString Application::userAgent()
{
    return QStringLiteral("Zeal/%1").arg(QCoreApplication::applicationVersion());
}

QString Application::userAgentJson() const
{
    QJsonObject app = {
        {QStringLiteral("version"), QCoreApplication::applicationVersion()},
        {QStringLiteral("qt_version"), qVersion()},
        {QStringLiteral("install_id"), m_settings->installId}
    };

    QJsonObject os = {
        {QStringLiteral("arch"), QSysInfo::currentCpuArchitecture()},
        {QStringLiteral("name"), QSysInfo::prettyProductName()},
        {QStringLiteral("product_type"), QSysInfo::productType()},
        {QStringLiteral("product_version"), QSysInfo::productVersion()},
        {QStringLiteral("kernel_type"), QSysInfo::kernelType()},
        {QStringLiteral("kernel_version"), QSysInfo::kernelVersion()},
        {QStringLiteral("locale"), QLocale::system().name()}
    };

    QJsonObject ua = {
        {QStringLiteral("app"), app},
        {QStringLiteral("os"), os}
    };

    return QString::fromUtf8(QJsonDocument(ua).toJson(QJsonDocument::Compact));
}
