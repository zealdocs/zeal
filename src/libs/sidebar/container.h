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
    Q_DISABLE_COPY(Container)
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
