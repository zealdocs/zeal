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

#include "version.h"

#include <QStringList>

using namespace Zeal::Util;

Version::Version(uint major, uint minor, uint patch) :
    m_major(major),
    m_minor(minor),
    m_patch(patch)
{
}

Version::Version(const QString &str)
{
    m_valid = fromString(str);
}

bool Version::isValid() const
{
    return m_valid;
}

QString Version::toString() const
{
    return QStringLiteral("%1.%2.%3").arg(m_major).arg(m_minor).arg(m_patch);
}

bool Version::fromString(const QString &str)
{
    const QStringList parts = str.split(QLatin1Char('.'));
    if (parts.size() != 3)
        return false;

    bool ok;

    m_major = parts[0].toUInt(&ok);
    if (!ok)
        return false;

    m_minor = parts[1].toUInt(&ok);
    if (!ok)
        return false;

    m_patch = parts[2].toUInt(&ok);
    if (!ok)
        return false;

    return true;
}

namespace Zeal {
namespace Util {

bool operator==(const Version &lhs, const Version &rhs)
{
    return lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor && lhs.m_patch == rhs.m_patch;
}

bool operator<(const Version &lhs, const Version &rhs)
{
    if (lhs.m_major < rhs.m_major)
        return true;
    else if (lhs.m_major > rhs.m_major)
        return false;

    if (lhs.m_minor < rhs.m_minor)
        return true;
    else if (lhs.m_minor > rhs.m_minor)
        return false;

    if (lhs.m_patch < rhs.m_patch)
        return true;

    return false;
}

} // namespace Util
} // namespace Zeal
