/****************************************************************************
**
** Copyright (C) 2020 Oleg Shparber
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
