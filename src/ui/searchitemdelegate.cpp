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

#include "searchitemstyle.h"
#include "searchitemdelegate.h"

#include "registry/searchmodel.h"

#include <algorithm>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QSize>

using namespace Zeal;

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

/**
 * @brief SearchItemDelegate::paint
 * Draws the text for each list entry with a correct highlight rectangle.
 * We delegate drawing of the border to the base style
 * and draw the icons/text ourselves.

 * To make the border have accurate width we set the bold font
 * and manually clip the rectangle.
 */
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

    // Dry run paint to obtain QRect of the painted text.
    QRect paintedTextRect = paintText(painter, option, defaultFont, boldFont, false);
    paintBorder(painter, option, paintedTextRect);
    paintText(painter, option, defaultFont, boldFont);

    painter->restore();
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}

/**
 * @brief SearchItemDelegate::getPaintBorderOptions
 * Manipulates style options to draw the border correctly.
 * 1. Appends text so that single letter items are displayed correctly by searchitemstyle.
 * 2. Sets the font to bold so that unclipped width is larger than actual border width.
 * 3. Extends the right side of the border if the bolded text is wider.
 */
void SearchItemDelegate::paintBorder(
        QPainter *painter,
        const QStyleOptionViewItem &option_,
        QRect actualTextRect) const
{
    QStyleOptionViewItem option(option_);
    option.text = option.text + "_do_not_draw";
    option.rect.setRight(actualTextRect.right());

    ZealSearchItemStyle().drawControl(
                QStyle::CE_ItemViewItem, &option, painter, option.widget);
}

/**
 * @brief SearchItemDelegate::getTextPaintOptions
 * Return paint options for a single item.
 */
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

/**
 * @brief SearchItemDelegate::paintText
 * Paints the text and computes the border size.
 *
 * @param painter Painter that should paint the text.
 * @param option Style options with the text to paint.
 * @param defaultFont Font of the normal text.
 * @param boldFont Font of the bolded text.
 * @param paint If false then does just a dry run and computes the border.
 * @return QRect of the border of the painted text.
 */
QRect SearchItemDelegate::paintText(
        QPainter *painter,
        QStyleOptionViewItem &option,
        QFont defaultFont,
        QFont boldFont,
        bool paint) const
{
    const QFontMetrics metrics(defaultFont);
    const QFontMetrics metricsBold(boldFont);

    if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    QRect normalRect = QApplication::style()->subElementRect(
                QStyle::SE_ItemViewItemText, &option, option.widget);
    option.font = boldFont;
    QRect rect = QApplication::style()->subElementRect(
                QStyle::SE_ItemViewItemText, &option, option.widget);

    const QString elided = metrics.elidedText(option.text, option.textElideMode, rect.width());

    int from = 0;
    while (from < elided.size()) {
        const int to = elided.indexOf(m_highlight, from, Qt::CaseInsensitive);
        QString text;

        painter->setFont(defaultFont);
        if (to == -1) {
            text = elided.mid(from);
            if (paint)
                painter->drawText(rect, text);
            rect.setLeft(rect.left() + metrics.width(text));
            break;
        }

        text = elided.mid(from, to - from);
        if (paint)
            painter->drawText(rect, text);
        rect.setLeft(rect.left() + metrics.width(text));

        text = elided.mid(to, m_highlight.size());
        painter->setFont(boldFont);
        if (paint)
            painter->drawText(rect, text);
        rect.setLeft(rect.left() + metricsBold.width(text));

        from = to + m_highlight.size();
    }

    // Measure QRect of the border.
    const int margin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget);
    int textWidth = std::max(normalRect.width(), rect.left() - normalRect.left() + 2 * margin);
    return QRect(normalRect.left(), rect.top(), textWidth, rect.height());
}
