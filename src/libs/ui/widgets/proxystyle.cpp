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

        if (!paintsTabIcon(option, widget)) {
            QProxyStyle::drawControl(element, option, painter, widget);
            return;
        }

        const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option);

        QRect iconRect;
        const QStyleOptionTab labelOption = tabLabelOption(tab, widget, &iconRect);

        const QIcon::Mode mode = tab->state.testFlag(State_Enabled) ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State state = tab->state.testFlag(State_Selected) ? QIcon::On : QIcon::Off;
        tab->icon.paint(painter, iconRect, Qt::AlignCenter, mode, state);

        QProxyStyle::drawControl(element, &labelOption, painter, widget);
        return;
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

QRect ProxyStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    // QTabBar elides titles to this rect, so it must match the label.
    if (element == SE_TabBarTabText && paintsTabIcon(option, widget)) {
        const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option);
        const QStyleOptionTab labelOption = tabLabelOption(tab, widget);
        return QProxyStyle::subElementRect(element, &labelOption, widget);
    }

    return QProxyStyle::subElementRect(element, option, widget);
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

bool ProxyStyle::paintsTabIcon(const QStyleOption *option, const QWidget *widget) const
{
    // In document mode the macOS style places the icon beside the centered
    // text. With left-aligned labels the text would overlap it. Other styles
    // already put the icon at the leading edge.
    if (qobject_cast<const TabBar *>(widget) == nullptr
        || baseStyle()->name().compare(QStringLiteral("macos"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option);
    if (tab == nullptr || !tab->documentMode || tab->icon.isNull()) {
        return false;
    }

    return tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::RoundedSouth
        || tab->shape == QTabBar::TriangularNorth || tab->shape == QTabBar::TriangularSouth;
}

QStyleOptionTab ProxyStyle::tabLabelOption(const QStyleOptionTab *tab, const QWidget *widget, QRect *iconRect) const
{
    // Same gap between icon and text as QCommonStyle and QMacStyle.
    constexpr int IconSpacing = 4;

    QSize iconSize = tab->iconSize;
    if (!iconSize.isValid()) {
        const int extent = pixelMetric(PM_SmallIconSize, tab, widget);
        iconSize = QSize(extent, extent);
    }

    iconSize = tab->icon.actualSize(iconSize);

    QStyleOptionTab labelOption(*tab);
    labelOption.icon = QIcon();

    if (iconRect != nullptr) {
        // Put the icon where the text would start.
        const QRect textRect = visualRect(tab->direction,
                                          tab->rect,
                                          QProxyStyle::subElementRect(SE_TabBarTabText, &labelOption, widget));
        *iconRect = visualRect(tab->direction,
                               tab->rect,
                               QRect(QPoint(textRect.left(), textRect.center().y() - iconSize.height() / 2), iconSize));
    }

    const int iconExtent = iconSize.width() + IconSpacing;
    if (tab->direction == Qt::RightToLeft) {
        labelOption.rect.setRight(labelOption.rect.right() - iconExtent);
    } else {
        labelOption.rect.setLeft(labelOption.rect.left() + iconExtent);
    }

    return labelOption;
}

} // namespace Zeal::WidgetUi
