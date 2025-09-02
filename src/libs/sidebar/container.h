// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_SIDEBAR_CONTAINER_H
#define ZEAL_SIDEBAR_CONTAINER_H

#include <QWidget>

class QSplitter;

namespace Zeal {
namespace Sidebar {

class View;

// TODO: Implement view groups (alt. naming: tabs, pages) (move splitter into a group?).
class Container : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Container)
public:
    explicit Container(QWidget *parent = nullptr);
    ~Container() override;

    void addView(View *view);

private:
    QSplitter *m_splitter = nullptr;

    QList<View *> m_views;
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_CONTAINER_H
