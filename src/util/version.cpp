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
    return QString("%1.%2.%3").arg(m_major).arg(m_minor).arg(m_patch);
}

/*bool Version::operator>(const Version &other) const
{
    if (m_major > other.m_major)
        return true;
    else if (m_major < other.m_major)
        return false;

    if (m_minor > other.m_minor)
        return true;
    else if (m_minor < other.m_minor)
        return false;

    if (m_patch > other.m_patch)
        return true;

    return false;
}*/

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

