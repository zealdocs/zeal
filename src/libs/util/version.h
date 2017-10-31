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

#ifndef ZEAL_UTIL_VERSION_H
#define ZEAL_UTIL_VERSION_H

#include <QString>

namespace Zeal {
namespace Util {

// Based on Semantic Versioning (http://semver.org/)
// TODO: Add support for prerelease tags and build metadata

class Version
{
public:
    Version(uint major = 0, uint minor = 0, uint patch = 0);
    Version(const QString &str);

    bool isValid() const;
    QString toString() const;

    friend bool operator==(const Version &lhs, const Version &rhs);
    friend bool operator< (const Version &lhs, const Version &rhs);
    friend bool operator!=(const Version &lhs, const Version &rhs) { return !operator==(lhs, rhs); }
    friend bool operator> (const Version &lhs, const Version &rhs) { return  operator< (rhs, lhs); }
    friend bool operator<=(const Version &lhs, const Version &rhs) { return !operator> (lhs, rhs); }
    friend bool operator>=(const Version &lhs, const Version &rhs) { return !operator< (lhs, rhs); }

private:
    bool fromString(const QString &str);

    bool m_valid = true;

    uint m_major = 0;
    uint m_minor = 0;
    uint m_patch = 0;
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_VERSION_H
