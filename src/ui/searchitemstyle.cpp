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

QRect ZealSearchItemStyle::subElementRect(SubElement element, const QStyleOption *option,
                                          const QWidget *widget) const
{
    if (element == QStyle::SE_ItemViewItemText) {
        // do not draw text - delegate does it
        return QRect();
    } else {
        return QProxyStyle::subElementRect(element, option, widget);
    }
}
