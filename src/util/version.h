#ifndef VERSION_H
#define VERSION_H

#include <QString>

namespace Zeal {
namespace Util {

// Based on Semantic Versioning (http://semver.org/)
/// TODO: Add support for prerelease tags and build metadata

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

#endif // VERSION_H
