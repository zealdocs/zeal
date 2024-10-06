// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_PLIST_H
#define ZEAL_UTIL_PLIST_H

#include <QHash>
#include <QVariant>

namespace Zeal {
namespace Util {

class Plist : public QHash<QString, QVariant>
{
public:
    Plist() = default;

    bool read(const QString &fileName);
    bool hasError() const;

private:
    bool m_hasError = false;
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_PLIST_H
