/****************************************************************************
**
** Copyright (C) 2015-2018 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
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

#include "readableinterval.h"

#include <QStringBuilder>
#include <QStringList>

using namespace Zeal::Util;

namespace {
const qint16 SECONDSPERMINUTE = 60;
const qint16 SECONDSPERHOUR = SECONDSPERMINUTE * 60;
const qint32 SECONDSPERDAY = SECONDSPERHOUR * 24;
const qint32 SECONDSPERYEAR = SECONDSPERDAY * 365;

const qint8 MAX_FIELDS_DISPLAYED = 3;

const QString ZERO_INTERVAL_STRING = "now";
const QString PAST_INTERVAL_STRING = "ago";
const QString FUTURE_INTERVAL_STRING = "from now";
const QString YEAR = "Year";
const QString DAY = "Day";
const QString HOUR = "Hour";
const QString MIN = "Minute";
const QString SEC = "Second";
const QString JOIN_SEQ= ", ";
}

QString ReadableInterval::pluralForm(const QString &word, qint64 quantity)
{
    return word + (quantity > 1 ? "s" : "");
}

QString ReadableInterval::toReadableString(const QDateTime& timestamp, const QDateTime& reference)
{
    qint64 delta, year, day, hour, min, sec;
    bool isPast;

    delta = reference.toSecsSinceEpoch() - timestamp.toSecsSinceEpoch();

    if (delta) {
        QStringList list;
        qint8 fieldCount = 0;

        isPast = delta > 0;
        year = delta / SECONDSPERYEAR;
        day = (delta % SECONDSPERYEAR) / SECONDSPERDAY;
        hour = ((delta % SECONDSPERYEAR) % SECONDSPERDAY) / SECONDSPERHOUR;
        min = (((delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) / SECONDSPERMINUTE;
        sec = (((delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) % SECONDSPERMINUTE;

        if (year && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(year).arg(ReadableInterval::pluralForm(YEAR, year)));
        if (day && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(day).arg(ReadableInterval::pluralForm(DAY, day)));
        if (hour && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(hour).arg(ReadableInterval::pluralForm(HOUR, hour)));
        if (min && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(min).arg(ReadableInterval::pluralForm(MIN, min)));
        if (sec && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(sec).arg(ReadableInterval::pluralForm(SEC, sec)));
        return QStringLiteral("%1 %2").arg(list.join(JOIN_SEQ), isPast ? PAST_INTERVAL_STRING : FUTURE_INTERVAL_STRING);
    } else {
        return ZERO_INTERVAL_STRING;
    }
}
