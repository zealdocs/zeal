/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "searchitemdelegate.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

bool SearchItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
                                   const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (sizeHint(option, index).width() <= view->visualRect(index).width()) {
        QToolTip::hideText();
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    QToolTip::showText(event->globalPos(), index.data().toString(), view);

    return true;
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_,
                               const QModelIndex &index) const
{
    if (m_highlight.isEmpty()) {
        QStyledItemDelegate::paint(painter, option_, index);
        return;
    }

    QStyleOptionViewItem option(option_);

    if (!index.data(Qt::DecorationRole).isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = index.data(Qt::DecorationRole).value<QIcon>();
    }

    QStyle *style = option.widget->style();
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    const QRect rect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget);

    // Match QCommonStyle behaviour.
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1;
    const QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0);

    const QString text = index.data().toString();
    const QFontMetrics fm(option.font);
    const QString elidedText = fm.elidedText(text, option.textElideMode, textRect.width());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QColor::fromRgb(255, 253, 0));

    const QColor highlightColor = option.state & (QStyle::State_Selected | QStyle::State_HasFocus)
            ? QColor::fromRgb(255, 255, 100, 20) : QColor::fromRgb(255, 255, 100, 120);

    for (int i = 0; i < elidedText.length();) {
        const int matchIndex = text.indexOf(m_highlight, i, Qt::CaseInsensitive);
        if (matchIndex == -1 || matchIndex >= elidedText.length() - 1)
            break;

        QRect highlightRect = textRect.adjusted(fm.width(elidedText.left(matchIndex)), 2, 0, -2);
        highlightRect.setWidth(fm.width(elidedText.mid(matchIndex, m_highlight.length())));

        QPainterPath path;
        path.addRoundedRect(highlightRect, 2, 2);

        painter->fillPath(path, highlightColor);
        painter->drawPath(path);

        i = matchIndex + m_highlight.length();
    }

    painter->restore();
    painter->save();

#ifdef Q_OS_WIN32
    // QWindowsVistaStyle overrides highlight colour.
    if (style->objectName() == QStringLiteral("windowsvista")) {
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
    }
#endif

    const QPalette::ColorGroup cg = option.state & QStyle::State_Active
            ? QPalette::Normal : QPalette::Inactive;

    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    else
        painter->setPen(option.palette.color(cg, QPalette::Text));

    // Vertically align the text in the middle to match QCommonStyle behaviour.
    const QRect alignedRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                                  QSize(1, fm.height()), textRect);
    painter->drawText(QPoint(alignedRect.x(), alignedRect.y() + fm.ascent()), elidedText);
    painter->restore();
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}
