#ifndef PLIST_H
#define PLIST_H

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

} // namespace Zeal
} // namespace Util

#endif // PLIST_H
