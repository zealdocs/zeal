// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "humanizer.h"

#include <QDateTime>

#include <cmath>

using namespace Zeal::Util;

namespace {
// Time thresholds in seconds.
constexpr double Minute = 60.0;
constexpr double Hour = 3600.0;
constexpr double Day = 86400.0;
// constexpr double Week = 604800.0; // Not used at the moment.
constexpr double Month = 2629746.0; // Average month (30.44 days)
constexpr double Year = 31556952.0; // Average year (365.25 days)

QString humanizeDuration(double seconds)
{
    if (seconds < 45) {
        return Humanizer::tr("a few seconds");
    } else if (seconds < 90) {
        return Humanizer::tr("a minute");
    } else if (seconds < 45 * Minute) {
        const int minutes = static_cast<int>(std::round(seconds / Minute));
        // return Humanizer::tr("%n minute(s)", nullptr, minutes);
        return Humanizer::tr("%1 minutes").arg(minutes);
    } else if (seconds < 90 * Minute) {
        return Humanizer::tr("an hour");
    } else if (seconds < 22 * Hour) {
        const int hours = static_cast<int>(std::round(seconds / Hour));
        // return Humanizer::tr("%n hour(s)", nullptr, hours);
        return Humanizer::tr("%1 hours").arg(hours);
    } else if (seconds < 36 * Hour) {
        return Humanizer::tr("a day");
    } else if (seconds < 25 * Day) {
        const int days = static_cast<int>(std::round(seconds / Day));
        // return Humanizer::tr("%n day(s)", nullptr, days);
        return Humanizer::tr("%1 days").arg(days);
    } else if (seconds < 45 * Day) {
        return Humanizer::tr("a month");
    } else if (seconds < 320 * Day) {
        const int months = static_cast<int>(std::round(seconds / Month));
        // return Humanizer::tr("%n month(s)", nullptr, months);
        return Humanizer::tr("%1 months").arg(months);
    } else if (seconds < 548 * Day) {
        return Humanizer::tr("a year");
    } else {
        const int years = static_cast<int>(std::round(seconds / Year));
        // return Humanizer::tr("%n year(s)", nullptr, years);
        return Humanizer::tr("%1 years").arg(years);
    }
}
} // namespace

QString Humanizer::fromNow(const QDateTime& dt)
{
    const double seconds = QDateTime::currentDateTime().secsTo(dt);
    const QString humanizedDuration = humanizeDuration(std::abs(seconds));

    return  (seconds > 0 ? tr("%1 from now") : tr("%1 ago")).arg(humanizedDuration);
}
