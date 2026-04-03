// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_CASEINSENSITIVEMAP_H
#define ZEAL_UTIL_CASEINSENSITIVEMAP_H

#include <QString>

#include <map>

namespace Zeal::Util {

struct CaseInsensitiveStringComparator
{
    bool operator()(const QString &lhs, const QString &rhs) const
    {
        return QString::compare(lhs, rhs, Qt::CaseInsensitive) < 0;
    }
};

template<typename T>
using CaseInsensitiveMap = std::map<QString, T, CaseInsensitiveStringComparator>;

} // namespace Zeal::Util

#endif // ZEAL_UTIL_CASEINSENSITIVEMAP_H
