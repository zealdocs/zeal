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

#include "registry/searchmodel.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QSize>

using namespace Zeal;

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

// HACK: allows to draw rich text for highlight.
// We delegate drawing of the border to the base style
// and draw the icons/text ourselves.

// To make the border have accurate width we pass fake options with
// an icon having an intended width to the base style.
// Therefore we first need to compute the expected width,
// then draw the border, then draw the rest.
void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_,
                               const QModelIndex &index) const
{
    if (m_highlight.isEmpty()) {
        QStyledItemDelegate::paint(painter, option_, index);
        return;
    }

    painter->save();

    QStyleOptionViewItem option = getTextPaintOptions(
                option_,
                index.data(Qt::DisplayRole).toString(),
                index.data(Qt::DecorationRole).value<QIcon>());
    option.font = painter->font();

    const QFont defaultFont(option.font);
    QFont boldFont(option.font);
    boldFont.setBold(true);
    const QFontMetrics metrics(defaultFont);
    const QFontMetrics metricsBold(boldFont);

    int boldWidth = metricsBold.boundingRect(m_highlight).width() - metrics.boundingRect(m_highlight).width();
    int width = metrics.boundingRect(option.text).width() + boldWidth;

    QStyleOptionViewItem borderOption = getPaintBorderOptions(option, 16, width);
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &borderOption, painter, option.widget);

    QRect iconRect = QApplication::style()->subElementRect(
                QStyle::SE_ItemViewItemDecoration, &option, option.widget);
    option.icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);

    paintText(painter, option, defaultFont, boldFont, metrics, metricsBold, boldWidth);

    painter->restore();
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}

QStyleOptionViewItem SearchItemDelegate::getPaintBorderOptions(
        const QStyleOptionViewItem &option_,
        int iconWidth,
        int textWidth) const
{
    QStyleOptionViewItem option(option_);
    option.text = "";
    option.icon = QIcon(QPixmap(1, 1));
    option.decorationSize = QSize(iconWidth + textWidth, 16);
    return option;
}

QStyleOptionViewItem SearchItemDelegate::getTextPaintOptions(
        const QStyleOptionViewItem &option_,
        QString searchText,
        QIcon icon) const
{
    QStyleOptionViewItem option(option_);
    option.text = searchText;
    option.features |= QStyleOptionViewItem::HasDisplay;

    if (!icon.isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = icon;
    }
    return option;
}

void SearchItemDelegate::paintText(
        QPainter *painter,
        QStyleOptionViewItem &option,
        QFont defaultFont,
        QFont boldFont,
        QFontMetrics metrics,
        QFontMetrics metricsBold,
        int boldWidth) const
{
    if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    const int margin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget);
    QRect rect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &option,
                                                       option.widget);
    rect.adjust(margin, 0, 0, 0);
    rect.setRight(option.rect.width() - margin - boldWidth);

    const QString elided = metrics.elidedText(option.text, option.textElideMode, rect.width());

    int from = 0;
    while (from < elided.size()) {
        const int to = elided.indexOf(m_highlight, from, Qt::CaseInsensitive);

        painter->setFont(defaultFont);
        if (to == -1) {
            painter->drawText(rect, elided.mid(from));
            break;
        }

        QString text;

        text = elided.mid(from, to - from);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metrics.width(text));

        text = elided.mid(to, m_highlight.size());
        painter->setFont(boldFont);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metricsBold.width(text));

        from = to + m_highlight.size();
    }
}
