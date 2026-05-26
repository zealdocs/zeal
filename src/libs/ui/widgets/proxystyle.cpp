// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proxystyle.h"

#include "iconhelper.h"

#include <QIcon>
#include <QPainter>
#include <QStyleOption>

namespace Zeal::WidgetUi {

ProxyStyle::ProxyStyle(QStyle *baseStyle)
    : QProxyStyle(baseStyle)
{
}

void ProxyStyle::drawPrimitive(PrimitiveElement element,
                               const QStyleOption *option,
                               QPainter *painter,
                               const QWidget *widget) const
{
    // Restyle the tab bar close button with the Tabler glyph, deferring to a real
    // desktop icon theme when one is active (mirrors IconHelper::fromTheme).
    if (element == PE_IndicatorTabClose && !IconHelper::useThemeIcons()) {
        QIcon::Mode mode = QIcon::Normal;
        if (!option->state.testFlag(State_Enabled)) {
            mode = QIcon::Disabled;
        } else if (option->state.testAnyFlags(State_Sunken | State_MouseOver)) {
            mode = QIcon::Active;

            // Draw the base style's auto-raised tool-button panel so the button
            // gets the same hover/pressed highlight as the other toolbar buttons.
            QStyleOption panelOption(*option);
            panelOption.state |= State_AutoRaise;
            QProxyStyle::drawPrimitive(PE_PanelButtonTool, &panelOption, painter, widget);
        }

        const QIcon icon = IconHelper::fromTheme(QStringLiteral("window-close"),
                                                 QStringLiteral(":/icons/tabler/x.svg"));
        icon.paint(painter, option->rect, Qt::AlignCenter, mode, QIcon::Off);
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

} // namespace Zeal::WidgetUi
