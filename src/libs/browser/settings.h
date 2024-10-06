// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_SETTINGS_H
#define ZEAL_BROWSER_SETTINGS_H

#include <QObject>

class QWebEngineProfile;

namespace Zeal {

namespace Core {
class Settings;
}

namespace Browser {

class Settings final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Settings)
public:
    explicit Settings(Core::Settings *appSettings, QObject *parent = nullptr);

    static QWebEngineProfile *defaultProfile();

private slots:
    void applySettings();

private:
    void setCustomStyleSheet(const QString &name, const QString &cssUrl);

    Core::Settings *m_appSettings = nullptr;
    static QWebEngineProfile *m_webProfile;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_SETTINGS_H
