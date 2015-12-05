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

#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <QString>

namespace Zeal {

class Docset;

struct SearchResult
{
    QString name;
    QString parentName;
    QString type;

    Docset *docset;

    QString path;

    /// TODO: Remove
    QString query;

    int score;

    bool operator<(const SearchResult &r) const;
};

} // namespace Zeal

#endif // SEARCHRESULT_H
