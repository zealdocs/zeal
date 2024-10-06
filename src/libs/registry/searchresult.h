/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_REGISTRY_SEARCHRESULT_H
#define ZEAL_REGISTRY_SEARCHRESULT_H

#include <QString>
#include <QUrl>

namespace Zeal {
namespace Registry {

class Docset;

struct SearchResult
{
    QString name;
    QString type;

    QString urlPath;
    QString urlFragment;

    Docset *docset;

    int score;

    inline bool operator<(const SearchResult &other) const
    {
        if (score == other.score)
            return QString::compare(name, other.name, Qt::CaseInsensitive) < 0;
        return score > other.score;
    }
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_SEARCHRESULT_H
