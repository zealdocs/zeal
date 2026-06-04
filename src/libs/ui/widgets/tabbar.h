// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TABBAR_H
#define ZEAL_WIDGETUI_TABBAR_H

#include <QTabBar>

namespace Zeal::WidgetUi {

// A tab bar with browser-like tab sizing: tabs share the available width
// equally, starting at the maximum width and shrinking as tabs are added.
class TabBar final : public QTabBar
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TabBar)
public:
    explicit TabBar(QWidget *parent = nullptr);
    ~TabBar() override = default;

protected:
    QSize tabSizeHint(int index) const override;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_TABBAR_H
