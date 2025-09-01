// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_SETTINGS_H
#define ZEAL_CORE_SETTINGS_H

#include <QDataStream>
#include <QKeySequence>
#include <QObject>

class QSettings;

namespace Zeal {
namespace Core {

class Settings final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Settings)
public:
    /* This public members are here just for simplification and should go away
     * once a more advanced settings management come in place.
     */

    // Startup
    bool startMinimized;
    bool checkForUpdate;
    bool hideMenuBar;
    // TODO: bool restoreLastState;

    // System Tray
    bool showSystrayIcon;
    bool minimizeToSystray;
    bool hideOnClose;

    // Global Shortcuts
    QKeySequence showShortcut;
    // TODO: QKeySequence searchSelectedTextShortcut;

    // Tabs Behavior
    bool openNewTabAfterActive;

    // Search
    bool isFuzzySearchEnabled;

    // Content
    QString defaultFontFamily;
    QString serifFontFamily;
    QString sansSerifFontFamily;
    QString fixedFontFamily;

    int defaultFontSize;
    int defaultFixedFontSize;
    int minimumFontSize;

    enum class ExternalLinkPolicy : unsigned int {
        Ask = 0,
        Open,
        OpenInSystemBrowser
    };
    Q_ENUM(ExternalLinkPolicy)
    ExternalLinkPolicy externalLinkPolicy = ExternalLinkPolicy::Ask;

    enum class ContentAppearance : unsigned int {
        Automatic = 0,
        Light,
        Dark
    };
    Q_ENUM(ContentAppearance)
    ContentAppearance contentAppearance = ContentAppearance::Automatic;

    bool isHighlightOnNavigateEnabled;
    QString customCssFile;
    bool isSmoothScrollingEnabled;

    // Network
    enum ProxyType : unsigned int {
        None = 0,
        System = 1,
        Http = 3,
        Socks5 = 4
    };
    Q_ENUM(ProxyType)

    // Internal
    // --------
    // InstallId is a UUID used to identify a Zeal installation. Created on first start or after
    // a settings wipe. It is not attached to user hardware or software, and is sent exclusively
    // to *.zealdocs.org hosts.
    QString installId;

    ProxyType proxyType = ProxyType::System;
    QString proxyHost;
    quint16 proxyPort;
    bool proxyAuthenticate;
    QString proxyUserName;
    QString proxyPassword;
    bool isIgnoreSslErrorsEnabled;

    // Other
    QString docsetPath;

    // State
    QByteArray windowGeometry;
    QByteArray verticalSplitterGeometry;
    QByteArray tocSplitterState;

    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;

    // Helper functions.
    bool isDarkModeEnabled() const;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    typedef Qt::ColorScheme ColorScheme;
#else
    enum class ColorScheme {
        Unknown,
        Light,
        Dark,
    };
#endif

    static ColorScheme colorScheme();

public slots:
    void load();
    void save();

signals:
    void updated();

private:
    void migrate(QSettings *settings) const;

    static QSettings *qsettings(QObject *parent = nullptr);
};

} // namespace Core
} // namespace Zeal

QDataStream &operator<<(QDataStream &out, Zeal::Core::Settings::ContentAppearance policy);
QDataStream &operator>>(QDataStream &in, Zeal::Core::Settings::ContentAppearance &policy);

QDataStream &operator<<(QDataStream &out, Zeal::Core::Settings::ExternalLinkPolicy policy);
QDataStream &operator>>(QDataStream &in, Zeal::Core::Settings::ExternalLinkPolicy &policy);

Q_DECLARE_METATYPE(Zeal::Core::Settings::ContentAppearance)
Q_DECLARE_METATYPE(Zeal::Core::Settings::ExternalLinkPolicy)

#endif // ZEAL_CORE_SETTINGS_H
