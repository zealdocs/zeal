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

#ifndef ZEAL_WIDGETUI_SIDEBARVIEWPROVIDER_H
#define ZEAL_WIDGETUI_SIDEBARVIEWPROVIDER_H

#include <sidebar/viewprovider.h>

namespace Zeal {
namespace WidgetUi {

class MainWindow;

class SidebarViewProvider : public Sidebar::ViewProvider
{
    Q_OBJECT
    Q_DISABLE_COPY(SidebarViewProvider)
public:
    explicit SidebarViewProvider(MainWindow *mainWindow);

    Sidebar::View *view(const QString &id = QString()) const override;

private:
    MainWindow *m_mainWindow = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SIDEBARVIEWPROVIDER_H
