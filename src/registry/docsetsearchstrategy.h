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

#ifndef DOCSETSEARCHSTRATEGY_H
#define DOCSETSEARCHSTRATEGY_H

#include "cancellationtoken.h"

#include <QString>
#include <QList>

namespace Zeal {

class SearchResult;

/// This class contains search related methods for docsets.
class DocsetSearchStrategy
{
public:
    DocsetSearchStrategy();
    virtual QList<SearchResult> search(const QString &query, CancellationToken token) = 0;

    /// Used to filter out cached results.
    virtual bool validResult(const QString &query, SearchResult previousResult,
                             SearchResult &result) = 0;
};

}

#endif // DOCSETSEARCHSTRATEGY_H
