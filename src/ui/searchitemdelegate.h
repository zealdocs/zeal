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

#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

class QSize;
class QStyleOptionViewItem;

namespace Zeal {

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void paintText(QPainter *painter, QStyleOptionViewItem &option, QFont defaultFont, QFont boldFont, QFontMetrics metrics, QFontMetrics metricsBold, int boldWidth) const;

public slots:
    void setHighlight(const QString &text);

private:
    QStyleOptionViewItem getPaintBorderOptions(const QStyleOptionViewItem &option, int iconWidth, int textWidth) const;
    QStyleOptionViewItem getTextPaintOptions(const QStyleOptionViewItem &option, QString searchText, QIcon icon) const;

    QString m_highlight;
};

}

#endif // SEARCHITEMDELEGATE_H
