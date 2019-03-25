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

#include "sidebarviewprovider.h"

#include "browsertab.h"
#include "mainwindow.h"
#include "searchsidebar.h"

using namespace Zeal;
using namespace Zeal::WidgetUi;

SidebarViewProvider::SidebarViewProvider(MainWindow *mainWindow)
    : Sidebar::ViewProvider(mainWindow)
    , m_mainWindow(mainWindow)
{
    connect(m_mainWindow, &MainWindow::currentTabChanged, this, &SidebarViewProvider::viewChanged, Qt::QueuedConnection);
}

Sidebar::View *SidebarViewProvider::view(const QString &id) const
{
    if (id == QLatin1String("index"))
        return m_mainWindow->currentTab()->searchSidebar();

    return nullptr;
}
