/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
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

#ifndef ZEAL_UTIL_PLIST_H
#define ZEAL_UTIL_PLIST_H

#include <QHash>
#include <QVariant>

namespace Zeal {
namespace Util {

class Plist : public QHash<QString, QVariant>
{
public:
    Plist();

    bool read(const QString &fileName);
    bool hasError() const;

private:
    bool m_hasError = false;
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_PLIST_H
