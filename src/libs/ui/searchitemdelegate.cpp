// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchitemdelegate.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <algorithm>

using namespace Zeal::WidgetUi;

SearchItemDelegate::SearchItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QList<int> SearchItemDelegate::decorationRoles() const
{
    return m_decorationRoles;
}

void SearchItemDelegate::setDecorationRoles(const QList<int> &roles)
{
    m_decorationRoles = roles;
}

int SearchItemDelegate::textHighlightRole() const
{
    return m_textHighlightRole;
}

void SearchItemDelegate::setTextHighlightRole(int role)
{
    m_textHighlightRole = role;
}

bool SearchItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
                                   const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() != QEvent::ToolTip)
        return QStyledItemDelegate::helpEvent(event, view, option, index);

    if (sizeHint(option, index).width() < view->visualRect(index).width()) {
        QToolTip::hideText();
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    QToolTip::showText(event->globalPos(), index.data().toString(), view);
    return true;
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    QStyle *style = opt.widget->style();

    // Find decoration roles with data present.
    QList<int> roles;
    for (int role : m_decorationRoles) {
        if (!index.data(role).isNull())
            roles.append(role);
    }

    // TODO: Implemented via initStyleOption() overload
    if (!roles.isEmpty()) {
        opt.features |= QStyleOptionViewItem::HasDecoration;
        opt.icon = index.data(roles.first()).value<QIcon>();

        // Constrain decoration size to the style's small icon size to ensure consistent icon sizing.
        const int maxSize = style->pixelMetric(QStyle::PM_SmallIconSize, &opt, opt.widget);
        opt.decorationSize = {std::min(opt.decorationSize.width(), maxSize),
                              std::min(opt.decorationSize.height(), maxSize)};
    }

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1;

    if (!roles.isEmpty()) {
        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if (opt.state & QStyle::State_Selected)
            mode = QIcon::Selected;
        const QIcon::State state = (opt.state & QStyle::State_Open) ? QIcon::On : QIcon::Off;

        // All icons are sized after the first one.
        QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
        // Undo RTL mirroring
        iconRect = style->visualRect(opt.direction, opt.rect, iconRect);
        const int dx = iconRect.width() + margin;

        for (int i = 1; i < roles.size(); ++i) {
            opt.decorationSize.rwidth() += dx;
            iconRect.translate(dx, 0);
            // Redo RTL mirroring
            const auto iconVisualRect = style->visualRect(opt.direction, opt.rect, iconRect);

            const QIcon icon = index.data(roles[i]).value<QIcon>();
            icon.paint(painter, iconVisualRect, opt.decorationAlignment, mode, state);
        }
    }

    // This should not happen unless a docset is corrupted.
    if (index.data().isNull())
        return;

    // Match QCommonStyle behavior.
    opt.features |= QStyleOptionViewItem::HasDisplay;
    opt.text = index.data().toString();
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget)
            .adjusted(margin, 0, -margin, 0);
    const QFontMetrics &fm = opt.fontMetrics;
    const QString elidedText = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

    // Get pre-computed match positions from model for highlighting.
    const QVector<int> matchPositions = index.data(m_textHighlightRole).value<QVector<int>>();
    if (!matchPositions.isEmpty()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QColor::fromRgb(255, 253, 0));

        const QColor highlightColor
                = (opt.state & (QStyle::State_Selected | QStyle::State_HasFocus))
                ? QColor::fromRgb(255, 255, 100, 20) : QColor::fromRgb(255, 255, 100, 120);

        {
            // Group consecutive positions to reduce number of highlight rectangles
            const int matchCount = matchPositions.size();

            int startPos = matchPositions[0];
            int endPos = matchPositions[0];

            for (int i = 1; i <= matchCount; ++i) {
                const bool isLast = (i == matchCount);
                const bool isConsecutive = !isLast && (matchPositions[i] == endPos + 1);

                if (isConsecutive) {
                    endPos = matchPositions[i];
                    continue;
                }

                // Draw highlight for [startPos, endPos]
                // Use FULL text (opt.text) for position calculation, not elided text
                const int highlightStart = startPos;
                const int highlightLen = endPos - startPos + 1;

                QRect highlightRect = textRect.adjusted(fm.horizontalAdvance(opt.text.left(highlightStart)), 2, 0, -2);
                highlightRect.setWidth(fm.horizontalAdvance(opt.text.mid(highlightStart, highlightLen)));

                // Clip highlight to visible text area (handles elided text correctly)
                highlightRect = highlightRect.intersected(textRect.adjusted(0, 2, 0, -2));

                // Only draw if rectangle is valid after clipping
                if (highlightRect.isValid() && !highlightRect.isEmpty()) {
                    QPainterPath path;
                    path.addRoundedRect(highlightRect, 2, 2);
                    painter->fillPath(path, highlightColor);
                    painter->drawPath(path);
                }

                if (!isLast) {
                    startPos = matchPositions[i];
                    endPos = matchPositions[i];
                }
            }
        }

        painter->restore();
    }

    painter->save();

#ifdef Q_OS_WINDOWS
    // QWindowsVistaStyle overrides highlight color.
    if (style->objectName() == QLatin1String("windowsvista")) {
        opt.palette.setColor(QPalette::All, QPalette::HighlightedText,
                             opt.palette.color(QPalette::Active, QPalette::Text));
    }
#endif

    const QPalette::ColorGroup cg = (opt.state & QStyle::State_Active)
            ? QPalette::Normal : QPalette::Inactive;

    if (opt.state & QStyle::State_Selected)
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    else
        painter->setPen(opt.palette.color(cg, QPalette::Text));

    // Constant LeftToRight because we don't need to flip it any further.
    // Vertically align the text in the middle to match QCommonStyle behaviour.
    const auto alignedRect = QStyle::alignedRect(Qt::LeftToRight, opt.displayAlignment,
                                                 QSize(textRect.size().width(), fm.height()), textRect);
    const auto textPoint = QPoint(alignedRect.x(), alignedRect.y() + fm.ascent());
    // Force LTR, so that BiDi won't reorder ellipsis to the left.
    painter->drawText(textPoint, elidedText, Qt::TextFlag::TextForceLeftToRight, 0);
    painter->restore();
}

QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    QStyle *style = opt.widget->style();

    // Constrain decoration size to the style's small icon size to ensure consistent icon sizing.
    const int maxSize = style->pixelMetric(QStyle::PM_SmallIconSize, &opt, opt.widget);
    opt.decorationSize = {std::min(opt.decorationSize.width(), maxSize),
                          std::min(opt.decorationSize.height(), maxSize)};

    QSize size = QStyledItemDelegate::sizeHint(opt, index);
    size.setWidth(0);

    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1;

    // Find decoration roles with data present.
    QList<int> roles;
    for (int role : m_decorationRoles) {
        if (!index.data(role).isNull())
            roles.append(role);
    }

    if (!roles.isEmpty()) {
        size.rwidth() = (opt.decorationSize.width() + margin) * roles.size() + margin;
    }

    size.rwidth() += opt.fontMetrics.horizontalAdvance(index.data().toString()) + margin * 2;

    return size;
}
