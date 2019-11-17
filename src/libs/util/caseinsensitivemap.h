/****************************************************************************
**
** Copyright (C) 2018 Oleg Shparber
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

#ifndef ZEAL_UTIL_CASEINSENSITIVEMAP_H
#define ZEAL_UTIL_CASEINSENSITIVEMAP_H

#include <QString>

#include <map>

namespace Zeal {
namespace Util {

struct CaseInsensitiveStringComparator
{
    bool operator()(const QString &lhs, const QString &rhs) const
    {
        return QString::compare(lhs, rhs, Qt::CaseInsensitive) < 0;
    }
};

template<typename T>
using CaseInsensitiveMap = std::map<QString, T, CaseInsensitiveStringComparator>;

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_CASEINSENSITIVEMAP_H
