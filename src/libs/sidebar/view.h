// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_SIDEBAR_VIEW_H
#define ZEAL_SIDEBAR_VIEW_H

#include <QWidget>

namespace Zeal {
namespace Sidebar {

class View : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(View)
public:
    explicit View(QWidget *parent = nullptr);
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_VIEW_H
