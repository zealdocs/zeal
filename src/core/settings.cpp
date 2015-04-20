#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#ifdef USE_WEBENGINE
    #include <QWebEngineSettings>
    #define QWebSettings QWebEngineSettings
#else
    #include <QWebSettings>
#endif

using namespace Zeal::Core;

Settings::Settings(QObject *parent) :
    QObject(parent),
#ifndef PORTABLE_BUILD
    m_settings(new QSettings(this))
#else
    m_settings(new QSettings(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"),
                             QSettings::IniFormat, this))
#endif
{
    /// TODO: Move to user style sheet (related to #268)
#ifndef USE_WEBENGINE
    QWebSettings::globalSettings()->setUserStyleSheetUrl(QStringLiteral("qrc:///browser/highlight.css"));
#endif

    load();
}

Settings::~Settings()
{
    save();
}

void Settings::load()
{
    /// TODO: Put everything in groups
    startMinimized = m_settings->value(QStringLiteral("start_minimized"), false).toBool();

    showSystrayIcon = m_settings->value(QStringLiteral("show_systray_icon"), true).toBool();
    minimizeToSystray = m_settings->value(QStringLiteral("minimize_to_systray"), false).toBool();
    hideOnClose = m_settings->value(QStringLiteral("hide_on_close"), false).toBool();

    m_settings->beginGroup(QStringLiteral("global_shortcuts"));
#ifndef Q_OS_OSX
    showShortcut = m_settings->value(QStringLiteral("show"), QStringLiteral("Meta+Z")).value<QKeySequence>();
#else
    showShortcut = m_settings->value(QStringLiteral("show"), QStringLiteral("Alt+Space")).value<QKeySequence>();
#endif
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("browser"));
    minimumFontSize = m_settings->value(QStringLiteral("minimum_font_size"),
                                        QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("proxy"));
    proxyType = static_cast<ProxyType>(m_settings->value(QStringLiteral("type"), ProxyType::System).toUInt());
    proxyHost = m_settings->value(QStringLiteral("host")).toString();
    proxyPort = m_settings->value(QStringLiteral("port"), 0).toInt();
    proxyAuthenticate = m_settings->value(QStringLiteral("authenticate"), false).toBool();
    proxyUserName = m_settings->value(QStringLiteral("username")).toString();
    proxyPassword = m_settings->value(QStringLiteral("password")).toString();
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("docsets"));
    if (m_settings->contains(QStringLiteral("path"))) {
        docsetPath = m_settings->value(QStringLiteral("path")).toString();
    } else {
#ifndef PORTABLE_BUILD
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + QLatin1String("/docsets");
#else
        docsetPath = QCoreApplication::applicationDirPath() + QLatin1String("/docsets");
#endif
        QDir().mkpath(docsetPath);
    }
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("state"));
    windowGeometry = m_settings->value(QStringLiteral("window_geometry")).toByteArray();
    splitterGeometry = m_settings->value(QStringLiteral("splitter_geometry")).toByteArray();
    m_settings->endGroup();
}

void Settings::save()
{
    /// TODO: Put everything in groups
    m_settings->setValue(QStringLiteral("start_minimized"), startMinimized);

    m_settings->setValue(QStringLiteral("show_systray_icon"), showSystrayIcon);
    m_settings->setValue(QStringLiteral("minimize_to_systray"), minimizeToSystray);
    m_settings->setValue(QStringLiteral("hide_on_close"), hideOnClose);

    m_settings->beginGroup(QStringLiteral("global_shortcuts"));
    m_settings->setValue(QStringLiteral("show"), showShortcut);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("browser"));
    m_settings->setValue(QStringLiteral("minimum_font_size"), minimumFontSize);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("proxy"));
    m_settings->setValue(QStringLiteral("type"), proxyType);
    m_settings->setValue(QStringLiteral("host"), proxyHost);
    m_settings->setValue(QStringLiteral("port"), proxyPort);
    m_settings->setValue(QStringLiteral("authenticate"), proxyAuthenticate);
    m_settings->setValue(QStringLiteral("username"), proxyUserName);
    m_settings->setValue(QStringLiteral("password"), proxyPassword);
    m_settings->endGroup();

#ifndef PORTABLE_BUILD
    m_settings->beginGroup(QStringLiteral("docsets"));
    m_settings->setValue(QStringLiteral("path"), docsetPath);
    m_settings->endGroup();
#endif

    m_settings->beginGroup(QStringLiteral("state"));
    m_settings->setValue(QStringLiteral("window_geometry"), windowGeometry);
    m_settings->setValue(QStringLiteral("splitter_geometry"), splitterGeometry);
    m_settings->endGroup();

    m_settings->sync();

    emit updated();
}

