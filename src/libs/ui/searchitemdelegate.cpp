/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
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

#include "searchitemdelegate.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

using namespace Zeal::WidgetUi;

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
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

        const QSize actualSize = opt.icon.actualSize(opt.decorationSize);
        opt.decorationSize = {std::min(opt.decorationSize.width(), actualSize.width()),
                              std::min(opt.decorationSize.height(), actualSize.height())};
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
        const int dx = iconRect.width() + margin;

        for (int i = 1; i < roles.size(); ++i) {
            opt.decorationSize.rwidth() += dx;
            iconRect.translate(dx, 0);

            const QIcon icon = index.data(roles[i]).value<QIcon>();
            icon.paint(painter, iconRect, opt.decorationAlignment, mode, state);
        }
    }

    // This should not happen unless a docset is corrupted.
    if (index.data().isNull())
        return;

    // Match QCommonStyle behaviour.
    opt.features |= QStyleOptionViewItem::HasDisplay;
    opt.text = index.data().toString();
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget)
            .adjusted(margin, 0, -margin, 0);
    const QFontMetrics &fm = opt.fontMetrics;
    const QString elidedText = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

    if (!m_highlight.isEmpty()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QColor::fromRgb(255, 253, 0));

        const QColor highlightColor
                = (opt.state & (QStyle::State_Selected | QStyle::State_HasFocus))
                ? QColor::fromRgb(255, 255, 100, 20) : QColor::fromRgb(255, 255, 100, 120);

        for (int i = 0;;) {
            const int matchIndex = opt.text.indexOf(m_highlight, i, Qt::CaseInsensitive);
            if (matchIndex == -1 || matchIndex >= elidedText.length() - 1)
                break;

            QRect highlightRect
                    = textRect.adjusted(fm.width(elidedText.left(matchIndex)), 2, 0, -2);
            highlightRect.setWidth(fm.width(elidedText.mid(matchIndex, m_highlight.length())));

            QPainterPath path;
            path.addRoundedRect(highlightRect, 2, 2);

            painter->fillPath(path, highlightColor);
            painter->drawPath(path);

            i = matchIndex + m_highlight.length();
        }

        painter->restore();
    }

    painter->save();

#ifdef Q_OS_WIN32
    // QWindowsVistaStyle overrides highlight colour.
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

    // Vertically align the text in the middle to match QCommonStyle behaviour.
    const QRect alignedRect = QStyle::alignedRect(opt.direction, opt.displayAlignment,
                                                  QSize(1, fm.height()), textRect);
    painter->drawText(QPoint(alignedRect.x(), alignedRect.y() + fm.ascent()), elidedText);
    painter->restore();
}

QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    QStyle *style = opt.widget->style();

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
        const QIcon icon = index.data(roles.first()).value<QIcon>();
        const QSize actualSize = icon.actualSize(opt.decorationSize);
        const int decorationWidth = std::min(opt.decorationSize.width(), actualSize.width());
        size.rwidth() = (decorationWidth + margin) * roles.size() + margin;
    }

    size.rwidth() += opt.fontMetrics.width(index.data().toString()) + margin * 2;
    return size;
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}
