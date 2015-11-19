/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QKeySequence>

class QSettings;

namespace Zeal {
namespace Core {

class Settings : public QObject
{
    Q_OBJECT
public:
    /// NOTE: This public members are here just for simplification and should go away
    /// once a more advanced settings management come in place.

    // Startup
    bool startMinimized;
    bool checkForUpdate;
    /// TODO: bool restoreLastState;

    // System Tray
    bool showSystrayIcon;
    bool minimizeToSystray;
    bool hideOnClose;

    // Global Shortcuts
    QKeySequence showShortcut;
    /// TODO: QKeySequence searchSelectedTextShortcut;

    // Browser
    int minimumFontSize;
    /// TODO: bool askOnExternalLink;
    /// TODO: QString customCss;

    // Network
    enum ProxyType : unsigned int {
        None,
        System,
        UserDefined
    };

    // Internal
    // --------
    // InstallId is a UUID used to indentify a Zeal installation. Created on first start or after
    // a settings wipe. It is not attached to user hardware or software, and is sent exclusevely
    // to *.zealdocs.org hosts.
    QString installId;
    // Version of configuration file format, should match Zeal version. Useful for migration rules.
    QString version;

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
    QByteArray splitterGeometry;

    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;

public slots:
    void load();
    void save();

signals:
    void updated();

private:
    QSettings *m_settings = nullptr;
};

} // namespace Core
} // namespace Zeal

Q_DECLARE_METATYPE(Zeal::Core::Settings::ProxyType)

#endif // SETTINGS_H
