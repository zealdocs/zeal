// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_HUMANIZER_H
#define ZEAL_UTIL_HUMANIZER_H

#include <QCoreApplication>
#include <QString>

class QDateTime;

namespace Zeal {
namespace Util {

class Humanizer
{
    Q_DECLARE_TR_FUNCTIONS(Humanizer)
    Q_DISABLE_COPY_MOVE(Humanizer)
    Humanizer() = delete;
    ~Humanizer() = delete;
public:
    static QString fromNow(const QDateTime& dt);
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_HUMANIZER_H
