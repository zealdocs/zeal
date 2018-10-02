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

namespace Zeal {
namespace Util {

class ReadableInterval
{
public:
    ReadableInterval(QDateTime timestamp, QDateTime reference);
    ReadableInterval(QDateTime timestamp);

    QString toReadableString();

private:
    void computeDateTimeComponents();
    QString pluralForm(QString word, qint64 quantity);

    QDateTime m_timestamp, m_reference;
    qint64 m_delta, m_year, m_day, m_hour, m_min, m_sec;
    bool m_isPast;

    const qint16 SECONDSPERMINUTE = 60;
    const qint16 SECONDSPERHOUR = SECONDSPERMINUTE * 60;
    const qint32 SECONDSPERDAY = SECONDSPERHOUR * 24;
    const qint32 SECONDSPERYEAR = SECONDSPERDAY * 365;

    const qint8 MAX_FIELDS_DISPLAYED = 3;

    QString ZERO_INTERVAL_STRING = "now";
    QString PAST_INTERVAL_STRING = "ago";
    QString FUTURE_INTERVAL_STRING = "from now";
    QString YEAR = "Year";
    QString DAY = "Day";
    QString HOUR = "Hour";
    QString MIN = "Minute";
    QString SEC = "Second";
    QString JOIN_SEQ= ", ";
};

} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_READABLEINTERVAL_H
