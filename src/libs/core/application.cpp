/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "application.h"

#include "extractor.h"
#include "networkaccessmanager.h"
#include "settings.h"

#include <registry/docsetregistry.h>
#include <registry/searchquery.h>
#include <ui/mainwindow.h>
#include <util/version.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QSysInfo>
#include <QThread>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
const char ReleasesApiUrl[] = "http://api.zealdocs.org/v1/releases";
}

Application *Application::m_instance = nullptr;

Application::Application(QObject *parent) :
    QObject(parent)
{
    // Ensure only one instance of Application
    Q_ASSERT(!m_instance);
    m_instance = this;

    m_settings = new Settings(this);
    m_networkManager = new NetworkAccessManager(this);

    // Extractor setup
    m_extractorThread = new QThread(this);
    m_extractor = new Extractor();
    m_extractor->moveToThread(m_extractorThread);
    m_extractorThread->start();
    connect(m_extractor, &Extractor::completed, this, &Application::extractionCompleted);
    connect(m_extractor, &Extractor::error, this, &Application::extractionError);
    connect(m_extractor, &Extractor::progress, this, &Application::extractionProgress);

    connect(m_settings, &Settings::updated, this, &Application::applySettings);
    applySettings();

    m_docsetRegistry = new Registry::DocsetRegistry();
    m_docsetRegistry->init(m_settings->docsetPath);

    m_mainWindow = new MainWindow(this);

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

void Application::executeQuery(const Registry::SearchQuery &query, bool preventActivation)
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

  Performs a check whether a new Zeal version is available. Setting \a quiet to true supresses
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

        const QJsonObject latestVersionInfo = jsonDoc.array().first().toObject();
        const Util::Version latestVersion = latestVersionInfo[QStringLiteral("version")].toString();
        if (latestVersion > Util::Version(QCoreApplication::applicationVersion()))
            emit updateCheckDone(latestVersion.toString());
        else if (!quiet)
            emit updateCheckDone();
    });
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

QString Application::userAgent()
{
    return QStringLiteral("Zeal/%1").arg(QCoreApplication::applicationVersion());
}

QString Application::userAgentJson() const
{
    // TODO: [Qt 5.4] Remove else branch
#if QT_VERSION >= 0x050400
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
#else
    QJsonObject app;
    app[QStringLiteral("version")] = QCoreApplication::applicationVersion();
    app[QStringLiteral("qt_version")] = QString::fromLatin1(qVersion());
    app[QStringLiteral("install_id")] = m_settings->installId;

    QJsonObject os;

#if defined(Q_PROCESSOR_ARM)
    os[QStringLiteral("arch")] = QStringLiteral("arm");
#elif defined(Q_PROCESSOR_X86_32)
    os[QStringLiteral("arch")] = QStringLiteral("i386");
#elif defined(Q_PROCESSOR_X86_64)
    os[QStringLiteral("arch")] = QStringLiteral("x86_64");
#else
    os[QStringLiteral("arch")] = QStringLiteral("unknown");
#endif // Q_PROCESSOR_*

    os[QStringLiteral("name")] = QStringLiteral("unknown");
    os[QStringLiteral("product_type")] = QStringLiteral("unknown");
    os[QStringLiteral("product_version")] = QStringLiteral("unknown");

#if defined(Q_OS_LINUX)
    os[QStringLiteral("kernel_type")] = QStringLiteral("linux");
#elif defined(Q_OS_WIN)
    os[QStringLiteral("kernel_type")] = QStringLiteral("windows");
#elif defined(Q_OS_OSX)
    os[QStringLiteral("kernel_type")] = QStringLiteral("osx");
#else
    os[QStringLiteral("kernel_type")] = QStringLiteral("unknown");
#endif // Q_OS_*

    os[QStringLiteral("kernel_version")] = QStringLiteral("unknown");
    os[QStringLiteral("locale")] = QLocale::system().name();

    QJsonObject ua;
    ua[QStringLiteral("app")] = app;
    ua[QStringLiteral("os")] = os;

#endif // QT_VERSION >= 0x050400

    return QString::fromUtf8(QJsonDocument(ua).toJson(QJsonDocument::Compact));
}
