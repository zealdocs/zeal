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

#include "docsetlistitemdelegate.h"

#include <registry/itemdatarole.h>
#include <registry/listmodel.h>

#include <QPainter>

using namespace Zeal;

DocsetListItemDelegate::DocsetListItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void DocsetListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QItemDelegate::paint(painter, option, index);

    if (!index.model()->data(index, Registry::ItemDataRole::UpdateAvailableRole).toBool())
        return;

    const QString text = tr("Update available");

    QFont font(painter->font());
    font.setItalic(true);

    const QFontMetrics fontMetrics(font);

    QRect textRect = option.rect;
    textRect.setLeft(textRect.right() - fontMetrics.width(text) - 2);

    painter->save();

    if (option.state & QStyle::State_Selected)
        painter->setPen(QPen(option.palette.highlightedText(), 1));

    painter->setFont(font);
    painter->drawText(textRect, text);

    painter->restore();
}
