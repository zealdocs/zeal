// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    if (id != QLatin1String("index")) {
        return nullptr;
    }

    if (auto tab = m_mainWindow->currentTab()) {
        return tab->searchSidebar();
    }

    return nullptr;
}
