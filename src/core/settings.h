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
    /// TODO: bool restoreLastState;

    // System Tray
    bool showSystrayIcon;
    bool minimizeToSystray;
    bool hideOnClose;

    // Global Shortcuts
    QKeySequence searchShortcut;
    /// TODO: QKeySequence searchSelectedTextShortcut;

    // Browser
    int minimumFontSize;
    /// TODO: bool askOnExternalLink;
    /// TODO: QString customCss;

    // Network
    enum ProxyType {
        None,
        System,
        UserDefined
    };

    ProxyType proxyType = ProxyType::System;
    QString proxyHost;
    quint16 proxyPort;
    bool proxyAuthenticate;
    QString proxyUserName;
    QString proxyPassword;

    // Other
    /// TODO: Convert into list
    QString docsetPath;

    // State
    QByteArray windowGeometry;
    QByteArray splitterGeometry;

    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;

public slots:
    void load();
    void save();

private:
    QSettings *m_settings = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // SETTINGS_H
