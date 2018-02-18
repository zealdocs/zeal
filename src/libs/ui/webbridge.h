/****************************************************************************
**
** Copyright (C) 2018 Oleg Shparber
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

#ifndef ZEAL_WIDGETUI_WEBBRIDGE_H
#define ZEAL_WIDGETUI_WEBBRIDGE_H

#include <QObject>

namespace Zeal {
namespace WidgetUi {

class WebBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString AppVersion READ appVersion CONSTANT)
public:
    explicit WebBridge(QObject *parent = nullptr);

signals:
    void actionTriggered(const QString &action);

public slots:
    Q_INVOKABLE void openShortUrl(const QString &key);
    Q_INVOKABLE void triggerAction(const QString &action);

private:
    QString appVersion() const;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_WEBBRIDGE_H
