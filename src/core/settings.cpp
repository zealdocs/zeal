#include "settings.h"

#include "registry/docsetsregistry.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QWebSettings>

using namespace Zeal::Core;

Settings::Settings(QObject *parent) :
    QObject(parent),
    m_settings(new QSettings(this))
{
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

    showSystrayIcon = m_settings->value(QStringLiteral("show_systray_icon"), false).toBool();
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
    minimumFontSize = m_settings->value("minimum_font_size", QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("proxy"));
    proxyType = static_cast<ProxyType>(m_settings->value("type", ProxyType::System).toUInt());
    proxyHost = m_settings->value("host").toString();
    proxyPort = m_settings->value("port", 0).toInt();
    proxyAuthenticate = m_settings->value("authenticate", false).toBool();
    proxyUserName = m_settings->value("username").toString();
    proxyPassword = m_settings->value("password").toString();
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("docsets"));
    if (m_settings->contains("path")) {
        docsetPath = m_settings->value("path").toString();
    } else {
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + QLatin1String("/docsets");
        QDir().mkpath(docsetPath);
    }
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("state"));
    windowGeometry = m_settings->value("window_geometry").toByteArray();
    splitterGeometry = m_settings->value("splitter_geometry").toByteArray();
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
    m_settings->setValue("minimum_font_size", minimumFontSize);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("proxy"));
    m_settings->setValue("type", proxyType);
    m_settings->setValue("host", proxyHost);
    m_settings->setValue("port", proxyPort);
    m_settings->setValue("authenticate", proxyAuthenticate);
    m_settings->setValue("username", proxyUserName);
    m_settings->setValue("password", proxyPassword);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("docsets"));
    m_settings->setValue("path", docsetPath);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("state"));
    m_settings->setValue("window_geometry", windowGeometry);
    m_settings->setValue("splitter_geometry", splitterGeometry);
    m_settings->endGroup();

    m_settings->sync();

    emit updated();
}

