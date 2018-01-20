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

#ifndef ZEAL_CORE_SETTINGS_H
#define ZEAL_CORE_SETTINGS_H

#include <QObject>
#include <QKeySequence>

class QSettings;

namespace Zeal {
namespace Core {

class Settings : public QObject
{
    Q_OBJECT
public:
    /* This public members are here just for simplification and should go away
     * once a more advanced settings management come in place.
     */

    // Startup
    bool startMinimized;
    bool checkForUpdate;
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
    bool fuzzySearchEnabled;

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

    bool darkModeEnabled;
    bool highlightOnNavigateEnabled;
    QString customCssFile;
    bool isSmoothScrollingEnabled;
    bool isAdDisabled;

    // Network
    enum ProxyType : unsigned int {
        None,
        System,
        UserDefined
    };
    Q_ENUM(ProxyType)

    // Internal
    // --------
    // InstallId is a UUID used to indentify a Zeal installation. Created on first start or after
    // a settings wipe. It is not attached to user hardware or software, and is sent exclusevely
    // to *.zealdocs.org hosts.
    QString installId;

    ProxyType proxyType = ProxyType::System;
    QString proxyHost;
    quint16 proxyPort;
    bool proxyAuthenticate;
    QString proxyUserName;
    QString proxyPassword;

    // Other
    QString docsetPath;

    // State
    QByteArray windowGeometry;
    QByteArray verticalSplitterGeometry;
    QByteArray tocSplitterState;

    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;

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

QDataStream &operator<<(QDataStream &out, const Zeal::Core::Settings::ExternalLinkPolicy &policy);
QDataStream &operator>>(QDataStream &in, Zeal::Core::Settings::ExternalLinkPolicy &policy);

Q_DECLARE_METATYPE(Zeal::Core::Settings::ExternalLinkPolicy)

#endif // ZEAL_CORE_SETTINGS_H
