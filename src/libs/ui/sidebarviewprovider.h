// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
