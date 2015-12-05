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

#include "searchresult.h"

using namespace Zeal;

bool SearchResult::operator<(const SearchResult &r) const
{
    if (score != r.score)
        return score > r.score;

    const bool lhsStartsWithQuery = name.startsWith(query, Qt::CaseInsensitive);
    const bool rhsStartsWithQuery = r.name.startsWith(query, Qt::CaseInsensitive);

    if (lhsStartsWithQuery != rhsStartsWithQuery)
        return lhsStartsWithQuery > rhsStartsWithQuery;

    const int namesCmp = QString::compare(name, r.name, Qt::CaseInsensitive);
    if (namesCmp)
        return namesCmp < 0;

    return QString::compare(parentName, r.parentName, Qt::CaseInsensitive) < 0;
}
