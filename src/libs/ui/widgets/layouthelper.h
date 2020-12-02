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

#ifndef ZEAL_WIDGETUI_LAYOUTHELPER_H
#define ZEAL_WIDGETUI_LAYOUTHELPER_H

#include <QLayout>

namespace Zeal {
namespace WidgetUi {

namespace LayoutHelper {

template<class Layout>
Layout *createBorderlessLayout()
{
    static_assert(std::is_base_of<QLayout, Layout>::value, "Layout must derive from QLayout");
    auto layout = new Layout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    return layout;
}

} // namespace LayoutHelper

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_LAYOUTHELPER_H
