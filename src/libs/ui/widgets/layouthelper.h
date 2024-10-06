/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
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
