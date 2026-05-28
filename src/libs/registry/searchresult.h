// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_SEARCHRESULT_H
#define ZEAL_REGISTRY_SEARCHRESULT_H

#include <QIcon>
#include <QList>
#include <QString>
#include <QUrl>

#include <compare>

namespace Zeal::Registry {

struct SearchResult
{
    QString name;
    QString type;

    QUrl url;

    QString docsetName;
    QIcon docsetIcon;

    double score = 0;
    QList<int> matchPositions;

    std::partial_ordering operator<=>(const SearchResult &other) const
    {
        if (const auto cmp = other.score <=> score; cmp != 0) {
            return cmp;
        }

        return QString::compare(name, other.name, Qt::CaseInsensitive) <=> 0;
    }

    bool operator==(const SearchResult &other) const
    {
        return score == other.score && QString::compare(name, other.name, Qt::CaseInsensitive) == 0;
    }
};

} // namespace Zeal::Registry

#endif // ZEAL_REGISTRY_SEARCHRESULT_H
