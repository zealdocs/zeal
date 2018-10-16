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

#include <QList>
#include <QStringList>

using namespace Zeal::Util;

const char* ReadableInterval::describeTimeUnit(TIME_UNITS unit)
{
    switch (unit) {
    case YEAR: return QT_TR_NOOP("%n year(s)");
    case DAY: return QT_TR_NOOP("%n day(s)");
    case HOUR: return QT_TR_NOOP("%n hour(s)");
    case MIN: return QT_TR_NOOP("%n min(s)");
    case SEC: return QT_TR_NOOP("%n sec(s)");
    }
    return QT_TR_NOOP("%n unit(s)");
}

QString ReadableInterval::pluralForm(TIME_UNITS unit, qint64 quantity)
{
    return tr(describeTimeUnit(unit), "", static_cast<int>(quantity));
}

QString ReadableInterval::toReadableString(const QDateTime& timestamp, const QDateTime& reference)
{
    qint64 delta, year, day, hour, min, sec;

    delta = (reference.toMSecsSinceEpoch() - timestamp.toMSecsSinceEpoch()) / MILLISECONDSPERSECOND;

    if (!delta) {
        return tr("now");
    }

    bool isPast = delta > 0;
    year = delta / SECONDSPERYEAR;
    day = (delta % SECONDSPERYEAR) / SECONDSPERDAY;
    hour = ((delta % SECONDSPERYEAR) % SECONDSPERDAY) / SECONDSPERHOUR;
    min = (((delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) / SECONDSPERMINUTE;
    sec = (((delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) % SECONDSPERMINUTE;

    QStringList list;
    QList<qint64> fields({year, day, hour, min, sec});
    QList<qint64> units({YEAR, DAY, HOUR, MIN, SEC});

    for (qint8 i = 0, j = 0; i < fields.length() && j <= MAX_FIELDS_DISPLAYED; ++i) {
        if (fields[i] && ++j) {
            list.append(pluralForm(static_cast<TIME_UNITS>(units[i]), fields[i]));
        }
    }

    return QStringLiteral("%1 %2").arg(list.join(tr(", ")),
                                       isPast ? tr("ago") : tr("from now"));
}
