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

ReadableInterval::ReadableInterval(QDateTime timestamp, QDateTime reference) :
    m_timestamp(timestamp),
    m_reference(reference)
{
    m_delta = m_reference.toSecsSinceEpoch() - m_timestamp.toSecsSinceEpoch();
}

ReadableInterval::ReadableInterval(QDateTime timestamp) :
    m_timestamp(timestamp)
{
    m_reference = QDateTime::currentDateTime();
    m_delta = m_reference.toSecsSinceEpoch() - m_timestamp.toSecsSinceEpoch();
}

QString ReadableInterval::pluralForm(QString word, qint64 quantity)
{
    return word + (quantity > 1 ? "s" : "");
}

void ReadableInterval::computeDateTimeComponents()
{
    m_isPast = m_delta > 0;
    m_year = m_delta / SECONDSPERYEAR;
    m_day = (m_delta % SECONDSPERYEAR) / SECONDSPERDAY;
    m_hour = ((m_delta % SECONDSPERYEAR) % SECONDSPERDAY) / SECONDSPERHOUR;
    m_min = (((m_delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) / SECONDSPERMINUTE;
    m_sec = (((m_delta % SECONDSPERYEAR) % SECONDSPERDAY) % SECONDSPERHOUR) % SECONDSPERMINUTE;
}

QString ReadableInterval::toReadableString()
{
    if (m_delta == 0)
        return ZERO_INTERVAL_STRING;
    else {
        QStringList list;
        qint8 fieldCount = 0;
        computeDateTimeComponents();
        if (m_year && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(m_year).arg(pluralForm(YEAR, m_year)));
        if (m_day && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(m_day).arg(pluralForm(DAY, m_day)));
        if (m_hour && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(m_hour).arg(pluralForm(HOUR, m_hour)));
        if (m_min && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(m_min).arg(pluralForm(MIN, m_min)));
        if (m_sec && ++fieldCount <= MAX_FIELDS_DISPLAYED)
            list.append(QStringLiteral("%1 %2").arg(m_sec).arg(pluralForm(SEC, m_sec)));
        return QStringLiteral("%1 %2").arg(list.join(JOIN_SEQ)).arg(m_isPast ? PAST_INTERVAL_STRING : FUTURE_INTERVAL_STRING);
    }
}
