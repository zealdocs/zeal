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
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (Q_UNLIKELY(index.model()->data(index, Registry::ItemDataRole::UpdateAvailableRole).toBool())) {
        opt.font.setBold(true);
    }

    const QWidget *widget = opt.widget;
    QStyle *style = widget->style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}
