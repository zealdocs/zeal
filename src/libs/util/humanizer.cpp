// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "humanizer.h"

#include <QDateTime>

#include <cmath>

namespace Zeal::Util {

namespace {
// Time thresholds in seconds.
constexpr double Minute = 60.0;
constexpr double Hour = 3600.0;
constexpr double Day = 86400.0;
// constexpr double Week = 604800.0; // Not used at the moment.
constexpr double Month = 2629746.0; // Average month (30.44 days)
constexpr double Year = 31556952.0; // Average year (365.25 days)

// Bucket boundaries mirror moment.js's fromNow() heuristics:
// https://momentjs.com/docs/#/displaying/fromnow/
// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
QString humanizeDuration(double seconds)
{
    if (seconds < 45) {
        return Humanizer::tr("a few seconds");
    }

    if (seconds < 90) {
        return Humanizer::tr("a minute");
    }

    if (seconds < 45 * Minute) {
        const int minutes = static_cast<int>(std::round(seconds / Minute));
        return Humanizer::tr("%1 minutes").arg(minutes);
    }

    if (seconds < 90 * Minute) {
        return Humanizer::tr("an hour");
    }

    if (seconds < 22 * Hour) {
        const int hours = static_cast<int>(std::round(seconds / Hour));
        return Humanizer::tr("%1 hours").arg(hours);
    }

    if (seconds < 36 * Hour) {
        return Humanizer::tr("a day");
    }

    if (seconds < 25 * Day) {
        const int days = static_cast<int>(std::round(seconds / Day));
        return Humanizer::tr("%1 days").arg(days);
    }

    if (seconds < 45 * Day) {
        return Humanizer::tr("a month");
    }

    if (seconds < 320 * Day) {
        const int months = static_cast<int>(std::round(seconds / Month));
        return Humanizer::tr("%1 months").arg(months);
    }

    if (seconds < 548 * Day) {
        return Humanizer::tr("a year");
    }

    const int years = static_cast<int>(std::round(seconds / Year));
    return Humanizer::tr("%1 years").arg(years);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
} // namespace

QString Humanizer::fromNow(const QDateTime &dt)
{
    const double seconds = static_cast<double>(QDateTime::currentDateTime().secsTo(dt));
    const QString humanizedDuration = humanizeDuration(std::abs(seconds));

    return (seconds > 0 ? tr("%1 from now") : tr("%1 ago")).arg(humanizedDuration);
}

} // namespace Zeal::Util
