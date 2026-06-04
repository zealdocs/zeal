// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proxystyle.h"

#include "iconhelper.h"
#include "tabbar.h"

#include <QIcon>
#include <QPainter>
#include <QScopedValueRollback>
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

        // Draw the glyph smaller than its hit area and faded at rest, so it
        // reads as secondary chrome until hovered.
        const int side = qMin(option->rect.width(), option->rect.height()) * 2 / 3;
        QRect glyphRect(0, 0, side, side);
        glyphRect.moveCenter(option->rect.center());

        painter->save();
        if (mode == QIcon::Normal) {
            painter->setOpacity(0.5);
        }
        icon.paint(painter, glyphRect, Qt::AlignCenter, mode, QIcon::Off);
        painter->restore();
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void ProxyStyle::drawControl(ControlElement element,
                             const QStyleOption *option,
                             QPainter *painter,
                             const QWidget *widget) const
{
    // The base style centers tab label text and QStyleOptionTab carries no
    // alignment field, so flag the label pass and adjust in drawItemText().
    if (element == CE_TabBarTabLabel && qobject_cast<const TabBar *>(widget) != nullptr) {
        const QScopedValueRollback<bool> rollback(m_leftAlignItemText, true);
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

void ProxyStyle::drawItemText(QPainter *painter,
                              const QRect &rect,
                              int flags,
                              const QPalette &pal,
                              bool enabled,
                              const QString &text,
                              QPalette::ColorRole textRole) const
{
    if (m_leftAlignItemText) {
        auto alignment = Qt::Alignment::fromInt(flags);
        alignment.setFlag(Qt::AlignHorizontal_Mask, false);
        alignment.setFlag(Qt::AlignLeading);
        flags = alignment.toInt();
    }

    QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
}

} // namespace Zeal::WidgetUi
