/****************************************************************************
**
** Copyright (C) 2015-2018 Oleg Shparber
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

#ifndef ZEAL_UTIL_READABLEINTERVAL_H
#define ZEAL_UTIL_READABLEINTERVAL_H

#include <QDateTime>
#include <QCoreApplication>

#include <chrono>

namespace  {
// TODO : remove with c++20
using days = std::chrono::duration<qint64, std::ratio<86400>>;
using years = std::chrono::duration<qint64, std::ratio<31556952>>;

enum TIME_UNITS {
    YEAR,
    DAY,
    HOUR,
    MIN,
    SEC
};

const qint8 MAX_FIELDS_DISPLAYED = 3;

} // namespace

namespace Zeal {
namespace Util {

class ReadableInterval
{
    Q_DECLARE_TR_FUNCTIONS(ReadableInterval)
public:
    static QString toReadableString(const QDateTime& timestamp, const QDateTime& reference = QDateTime::currentDateTime());
private:
    static const char* describeTimeUnit(TIME_UNITS unit);
    static QString pluralForm(TIME_UNITS unit, qint64 quantity);
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_READABLEINTERVAL_H
