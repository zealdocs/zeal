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

#include "progressitemdelegate.h"

#include <QPainter>
#include <QProgressBar>

ProgressItemDelegate::ProgressItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void ProgressItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (!index.model()->data(index, ShowProgressRole).toBool()) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    bool ok;
    const int value = index.model()->data(index, ValueRole).toInt(&ok);

    if (!ok) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    // Adjust maximum text width
    QStyleOptionViewItem styleOption = option;
    styleOption.rect.setRight(styleOption.rect.right() - progressBarWidth);

    // Size progress bar
    QScopedPointer<QProgressBar> renderer(new QProgressBar());
    renderer->resize(progressBarWidth, styleOption.rect.height());
    renderer->setRange(0, 100);
    renderer->setValue(value);

    const QString format = index.model()->data(index, FormatRole).toString();
    if (!format.isEmpty())
        renderer->setFormat(format);

    painter->save();

    // Paint progress bar
    painter->translate(styleOption.rect.topRight());
    renderer->render(painter);

    painter->restore();

    QItemDelegate::paint(painter, styleOption, index);
}
