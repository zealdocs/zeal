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

#include "searchitemstyle.h"
#include "registry/searchmodel.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

using namespace Zeal;

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_,
                               const QModelIndex &index) const
{
    if (m_highlight.isEmpty()) {
        QStyledItemDelegate::paint(painter, option_, index);
        return;
    }

    painter->save();

    QStyleOptionViewItem option(option_);
    option.text = index.data().toString();
    option.features |= QStyleOptionViewItem::HasDisplay;

    if (!index.data(Qt::DecorationRole).isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = index.data(Qt::DecorationRole).value<QIcon>();
    }

    ZealSearchItemStyle style;
    style.drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    QRect rect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &option,
                                                       option.widget);
    const int margin = style.pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget);
    rect.adjust(margin, 0, 2, 0); // +2px for bold text

    const QFont defaultFont(painter->font());
    QFont boldFont(defaultFont);
    boldFont.setBold(true);

    const QFontMetrics metrics(defaultFont);
    const QFontMetrics metricsBold(boldFont);

    const QString elided = metrics.elidedText(option.text, option.textElideMode, rect.width());

    int from = 0;
    while (from < elided.size()) {
        const int to = elided.indexOf(m_highlight, from, Qt::CaseInsensitive);

        if (to == -1) {
            painter->drawText(rect, elided.mid(from));
            break;
        }

        QString text = elided.mid(from, to - from);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metrics.width(text));

        text = elided.mid(to, m_highlight.size());
        painter->setFont(boldFont);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metricsBold.width(text));

        painter->setFont(defaultFont);

        from = to + m_highlight.size();
    }

    painter->restore();
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}
