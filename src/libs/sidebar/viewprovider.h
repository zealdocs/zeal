/****************************************************************************
**
** Copyright (C) 2019 Oleg Shparber
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

#ifndef ZEAL_SIDEBAR_VIEWPROVIDER_H
#define ZEAL_SIDEBAR_VIEWPROVIDER_H

#include <QObject>

namespace Zeal {
namespace Sidebar {

class View;

class ViewProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ViewProvider)
public:
    explicit ViewProvider(QObject *parent = nullptr);

    virtual View *view(const QString &id = QString()) const = 0;

signals:
    void viewChanged();
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_VIEWPROVIDER_H
