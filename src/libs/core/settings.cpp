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

#include "settings.h"

#include "application.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#else
#include <QPalette>
#endif

namespace {
// Configuration file groups
constexpr char GroupContent[] = "content";
constexpr char GroupDocsets[] = "docsets";
constexpr char GroupGlobalShortcuts[] = "global_shortcuts";
constexpr char GroupSearch[] = "search";
constexpr char GroupTabs[] = "tabs";
constexpr char GroupInternal[] = "internal";
constexpr char GroupState[] = "state";
constexpr char GroupProxy[] = "proxy";
} // namespace

using namespace Zeal::Core;

Settings::Settings(QObject *parent)
    : QObject(parent)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<ContentAppearance>("ContentAppearance");
    qRegisterMetaTypeStreamOperators<ExternalLinkPolicy>("ExternalLinkPolicy");
#else
    qRegisterMetaType<ContentAppearance>("ContentAppearance");
    qRegisterMetaType<ExternalLinkPolicy>("ExternalLinkPolicy");
#endif

    load();
}

Settings::~Settings()
{
    save();
}

bool Settings::isDarkModeEnabled() const
{
    if (contentAppearance == ContentAppearance::Dark) {
        return true;
    }

    if (contentAppearance == ContentAppearance::Automatic && colorScheme() == ColorScheme::Dark) {
        return true;
    }

    return false;
}

Zeal::Core::Settings::ColorScheme Settings::colorScheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QApplication::styleHints()->colorScheme();
#else
    // Pre Qt 6.5 detection from https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5.
    const QPalette p;
    if (p.color(QPalette::WindowText).lightness() > p.color(QPalette::Window).lightness()) {
        return ColorScheme::Dark;
    }

    return ColorScheme::Light;
#endif
}

void Settings::load()
{
    QScopedPointer<QSettings> settings(qsettings());
    migrate(settings.data());

    // TODO: Put everything in groups
    startMinimized = settings->value(QStringLiteral("start_minimized"), false).toBool();
    checkForUpdate = settings->value(QStringLiteral("check_for_update"), true).toBool();

    showSystrayIcon = settings->value(QStringLiteral("show_systray_icon"), true).toBool();
    minimizeToSystray = settings->value(QStringLiteral("minimize_to_systray"), false).toBool();
    hideOnClose = settings->value(QStringLiteral("hide_on_close"), false).toBool();

    settings->beginGroup(GroupGlobalShortcuts);
    showShortcut = settings->value(QStringLiteral("show")).value<QKeySequence>();
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    openNewTabAfterActive = settings->value(QStringLiteral("open_new_tab_after_active"), false).toBool();
    settings->endGroup();

    settings->beginGroup(GroupSearch);
    isFuzzySearchEnabled = settings->value(QStringLiteral("fuzzy_search_enabled"), false).toBool();
    settings->endGroup();

    settings->beginGroup(GroupContent);

    contentAppearance = settings->value(QStringLiteral("appearance"),
                                        QVariant::fromValue(ContentAppearance::Automatic)).value<ContentAppearance>();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    // Dark mode needs to be applied before Qt WebEngine is initialized.
    if (isDarkModeEnabled()) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--blink-settings=forceDarkModeEnabled=true,darkModeInversionAlgorithm=4");
    }
#endif

    // Fonts
    QWebEngineSettings *webSettings = QWebEngineProfile::defaultProfile()->settings();
    serifFontFamily = settings->value(QStringLiteral("serif_font_family"),
                                      webSettings->fontFamily(QWebEngineSettings::SerifFont)).toString();
    sansSerifFontFamily = settings->value(QStringLiteral("sans_serif_font_family"),
                                          webSettings->fontFamily(QWebEngineSettings::SansSerifFont)).toString();
    fixedFontFamily = settings->value(QStringLiteral("fixed_font_family"),
                                      webSettings->fontFamily(QWebEngineSettings::FixedFont)).toString();

    static const QMap<QString, QWebEngineSettings::FontFamily> fontFamilies = {
        {QStringLiteral("sans-serif"), QWebEngineSettings::SansSerifFont},
        {QStringLiteral("serif"), QWebEngineSettings::SerifFont},
        {QStringLiteral("monospace"), QWebEngineSettings::FixedFont}
    };

    defaultFontFamily = settings->value(QStringLiteral("default_font_family"),
                                        QStringLiteral("serif")).toString();

    // Fallback to the serif font family.
    if (!fontFamilies.contains(defaultFontFamily)) {
        defaultFontFamily = QStringLiteral("serif");
    }

    webSettings->setFontFamily(QWebEngineSettings::SansSerifFont, sansSerifFontFamily);
    webSettings->setFontFamily(QWebEngineSettings::SerifFont, serifFontFamily);
    webSettings->setFontFamily(QWebEngineSettings::FixedFont, fixedFontFamily);

    const QString defaultFontFamilyResolved = webSettings->fontFamily(fontFamilies.value(defaultFontFamily));
    webSettings->setFontFamily(QWebEngineSettings::StandardFont, defaultFontFamilyResolved);

    defaultFontSize = settings->value(QStringLiteral("default_font_size"),
                                      webSettings->fontSize(QWebEngineSettings::DefaultFontSize)).toInt();
    defaultFixedFontSize = settings->value(QStringLiteral("default_fixed_font_size"),
                                           webSettings->fontSize(QWebEngineSettings::DefaultFixedFontSize)).toInt();
    minimumFontSize = settings->value(QStringLiteral("minimum_font_size"),
                                      webSettings->fontSize(QWebEngineSettings::MinimumFontSize)).toInt();

    webSettings->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFontSize);
    webSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, defaultFixedFontSize);
    webSettings->setFontSize(QWebEngineSettings::MinimumFontSize, minimumFontSize);

    isHighlightOnNavigateEnabled = settings->value(QStringLiteral("highlight_on_navigate"), true).toBool();
    customCssFile = settings->value(QStringLiteral("custom_css_file")).toString();
    externalLinkPolicy = settings->value(QStringLiteral("external_link_policy"),
                                         QVariant::fromValue(ExternalLinkPolicy::Ask)).value<ExternalLinkPolicy>();
    isSmoothScrollingEnabled = settings->value(QStringLiteral("smooth_scrolling"), true).toBool();
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    proxyType = static_cast<ProxyType>(settings->value(QStringLiteral("type"),
                                                       ProxyType::System).toUInt());
    proxyHost = settings->value(QStringLiteral("host")).toString();
    proxyPort = static_cast<quint16>(settings->value(QStringLiteral("port"), 0).toUInt());
    proxyAuthenticate = settings->value(QStringLiteral("authenticate"), false).toBool();
    proxyUserName = settings->value(QStringLiteral("username")).toString();
    proxyPassword = settings->value(QStringLiteral("password")).toString();
    isIgnoreSslErrorsEnabled = settings->value(QStringLiteral("ignore_ssl_errors"), false).toBool();
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    if (settings->contains(QStringLiteral("path"))) {
        docsetPath = settings->value(QStringLiteral("path")).toString();
    } else {
#ifndef PORTABLE_BUILD
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                + QLatin1String("/docsets");
#else
        docsetPath = QStringLiteral("docsets");
#endif
    }
    settings->endGroup();

    // Create the docset storage directory if it doesn't exist.
    const QFileInfo fi(docsetPath);
    if (!fi.exists()) {
        // TODO: Report QDir::mkpath() errors.
        if (fi.isRelative()) {
            QDir().mkpath(QCoreApplication::applicationDirPath() + "/" + docsetPath);
        } else {
            QDir().mkpath(docsetPath);
        }
    }

    settings->beginGroup(GroupState);
    windowGeometry = settings->value(QStringLiteral("window_geometry")).toByteArray();
    verticalSplitterGeometry = settings->value(QStringLiteral("splitter_geometry")).toByteArray();
    tocSplitterState = settings->value(QStringLiteral("toc_splitter_state")).toByteArray();
    settings->endGroup();

    settings->beginGroup(GroupInternal);
    installId = settings->value(QStringLiteral("install_id"),
                                // Avoid curly braces (QTBUG-885)
                                QUuid::createUuid().toString().mid(1, 36)).toString();
    settings->endGroup();
}

void Settings::save()
{
    QScopedPointer<QSettings> settings(qsettings());

    // TODO: Put everything in groups
    settings->setValue(QStringLiteral("start_minimized"), startMinimized);
    settings->setValue(QStringLiteral("check_for_update"), checkForUpdate);

    settings->setValue(QStringLiteral("show_systray_icon"), showSystrayIcon);
    settings->setValue(QStringLiteral("minimize_to_systray"), minimizeToSystray);
    settings->setValue(QStringLiteral("hide_on_close"), hideOnClose);

    settings->beginGroup(GroupGlobalShortcuts);
    settings->setValue(QStringLiteral("show"), showShortcut);
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    settings->setValue(QStringLiteral("open_new_tab_after_active"), openNewTabAfterActive);
    settings->endGroup();

    settings->beginGroup(GroupSearch);
    settings->setValue(QStringLiteral("fuzzy_search_enabled"), isFuzzySearchEnabled);
    settings->endGroup();

    settings->beginGroup(GroupContent);
    settings->setValue(QStringLiteral("default_font_family"), defaultFontFamily);
    settings->setValue(QStringLiteral("serif_font_family"), serifFontFamily);
    settings->setValue(QStringLiteral("sans_serif_font_family"), sansSerifFontFamily);
    settings->setValue(QStringLiteral("fixed_font_family"), fixedFontFamily);

    settings->setValue(QStringLiteral("default_font_size"), defaultFontSize);
    settings->setValue(QStringLiteral("default_fixed_font_size"), defaultFixedFontSize);
    settings->setValue(QStringLiteral("minimum_font_size"), minimumFontSize);

    settings->setValue(QStringLiteral("appearance"), QVariant::fromValue(contentAppearance));
    settings->setValue(QStringLiteral("highlight_on_navigate"), isHighlightOnNavigateEnabled);
    settings->setValue(QStringLiteral("custom_css_file"), customCssFile);
    settings->setValue(QStringLiteral("external_link_policy"), QVariant::fromValue(externalLinkPolicy));
    settings->setValue(QStringLiteral("smooth_scrolling"), isSmoothScrollingEnabled);
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    settings->setValue(QStringLiteral("type"), proxyType);
    settings->setValue(QStringLiteral("host"), proxyHost);
    settings->setValue(QStringLiteral("port"), proxyPort);
    settings->setValue(QStringLiteral("authenticate"), proxyAuthenticate);
    settings->setValue(QStringLiteral("username"), proxyUserName);
    settings->setValue(QStringLiteral("password"), proxyPassword);
    settings->setValue(QStringLiteral("ignore_ssl_errors"), isIgnoreSslErrorsEnabled);
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    settings->setValue(QStringLiteral("path"), docsetPath);
    settings->endGroup();

    settings->beginGroup(GroupState);
    settings->setValue(QStringLiteral("window_geometry"), windowGeometry);
    settings->setValue(QStringLiteral("splitter_geometry"), verticalSplitterGeometry);
    settings->setValue(QStringLiteral("toc_splitter_state"), tocSplitterState);
    settings->endGroup();

    settings->beginGroup(GroupInternal);
    settings->setValue(QStringLiteral("install_id"), installId);
    // Version of configuration file format, should match Zeal version. Used for migration rules.
    settings->setValue(QStringLiteral("version"), Application::version().toString());
    settings->endGroup();

    settings->sync();

    emit updated();
}

/*!
 * \internal
 * \brief Migrates settings from older application versions.
 * \param settings QSettings object to update.
 *
 * The settings migration process relies on 'internal/version' option, that was introduced in the
 * release 0.2.0, so a missing option indicates pre-0.2 release.
 */
void Settings::migrate(QSettings *settings) const
{
    settings->beginGroup(GroupInternal);
    const auto version = QVersionNumber::fromString(settings->value(QStringLiteral("version")).toString());
    settings->endGroup();

    //
    // 0.6.0
    //

    // Unset content.default_fixed_font_size.
    // The causing bug was 0.6.1 (#903), but the incorrect setting still comes to haunt us (#1054).
    if (version == QVersionNumber(0, 6, 0)) {
        settings->beginGroup(GroupContent);
        settings->remove(QStringLiteral("default_fixed_font_size"));
        settings->endGroup();
    }

    //
    // Pre 0.4
    //

    // Rename 'browser' group into 'content'.
    if (version < QVersionNumber(0, 4, 0)) {
        settings->beginGroup(QStringLiteral("browser"));
        const QVariant tmpMinimumFontSize = settings->value(QStringLiteral("minimum_font_size"));
        settings->endGroup();
        settings->remove(QStringLiteral("browser"));

        if (tmpMinimumFontSize.isValid()) {
            settings->beginGroup(GroupContent);
            settings->setValue(QStringLiteral("minimum_font_size"), tmpMinimumFontSize);
            settings->endGroup();
        }
    }

    //
    // Pre 0.3
    //

    // Unset 'state/splitter_geometry', because custom styles were removed.
    if (version < QVersionNumber(0, 3, 0)) {
        settings->beginGroup(GroupState);
        settings->remove(QStringLiteral("splitter_geometry"));
        settings->endGroup();
    }
}

/*!
 * \internal
 * \brief Returns an initialized QSettings object.
 * \param parent Optional parent object.
 * \return QSettings object.
 *
 * QSettings is initialized according to build options, e.g. standard vs portable.
 * Caller is responsible for deleting the returned object.
 */
QSettings *Settings::qsettings(QObject *parent)
{
#ifndef PORTABLE_BUILD
    return new QSettings(parent);
#else
    return new QSettings(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"),
                         QSettings::IniFormat, parent);
#endif
}

QDataStream &operator<<(QDataStream &out, Settings::ContentAppearance policy)
{
    out << static_cast<std::underlying_type<Settings::ContentAppearance>::type>(policy);
    return out;
}

QDataStream &operator>>(QDataStream &in, Settings::ContentAppearance &policy)
{
    std::underlying_type<Settings::ContentAppearance>::type value;
    in >> value;
    policy = static_cast<Settings::ContentAppearance>(value);
    return in;
}

QDataStream &operator<<(QDataStream &out, Settings::ExternalLinkPolicy policy)
{
    out << static_cast<std::underlying_type<Settings::ExternalLinkPolicy>::type>(policy);
    return out;
}

QDataStream &operator>>(QDataStream &in, Settings::ExternalLinkPolicy &policy)
{
    std::underlying_type<Settings::ExternalLinkPolicy>::type value;
    in >> value;
    policy = static_cast<Settings::ExternalLinkPolicy>(value);
    return in;
}
