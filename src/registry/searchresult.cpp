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
    if (searchRelevancy.relevancy != r.searchRelevancy.relevancy)
        return searchRelevancy.relevancy > r.searchRelevancy.relevancy;

    if (searchRelevancy.matchType != r.searchRelevancy.matchType)
        return searchRelevancy.matchType > r.searchRelevancy.matchType;

    const int lhsLength = token.full.length();
    const int rhsLength = r.token.full.length();
    if (lhsLength != rhsLength)
        return lhsLength < rhsLength;

    const int namesCmp = QString::compare(token.name, r.token.name, Qt::CaseInsensitive);
    if (namesCmp)
        return namesCmp < 0;

    return QString::compare(token.parentName, r.token.parentName, Qt::CaseInsensitive) < 0;
}
