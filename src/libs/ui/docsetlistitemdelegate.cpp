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

#include <QPainter>

using namespace Zeal;
using namespace Zeal::WidgetUi;

DocsetListItemDelegate::DocsetListItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void DocsetListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if (!index.model()->data(index, Registry::ItemDataRole::UpdateAvailableRole).toBool())
        return;

    const QString text = tr("Update available");

    QFont font(painter->font());
    font.setItalic(true);

    const QFontMetrics fontMetrics(font);

    QRect textRect = option.rect;
    textRect.setLeft(textRect.right() - fontMetrics.width(text) - 2);

    painter->save();

    QPalette palette = option.palette;

#ifdef Q_OS_WIN32
    // QWindowsVistaStyle overrides highlight colour.
    if (option.widget->style()->objectName() == QLatin1String("windowsvista")) {
        palette.setColor(QPalette::All, QPalette::HighlightedText,
                         palette.color(QPalette::Active, QPalette::Text));
    }
#endif

    const QPalette::ColorGroup cg = (option.state & QStyle::State_Active)
            ? QPalette::Normal : QPalette::Inactive;

    if (option.state & QStyle::State_Selected) {
        painter->setPen(palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(palette.color(cg, QPalette::Text));
    }

    painter->setFont(font);
    painter->drawText(textRect, text);

    painter->restore();
}
