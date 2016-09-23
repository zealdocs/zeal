/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
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

#include "searchquery.h"

using namespace Zeal::Registry;

namespace {
const char prefixSeparator = ':';
const char keywordSeparator = ',';
}

SearchQuery::SearchQuery()
{
}

SearchQuery::SearchQuery(const QString &query, const QStringList &keywords) :
    m_query(query)
{
    setKeywords(keywords);
}

SearchQuery SearchQuery::fromString(const QString &str)
{
    const int sepAt = str.indexOf(prefixSeparator);
    const int next = sepAt + 1;

    QString query;
    QStringList keywords;
    if (sepAt > 0 && (next >= str.size() || str.at(next) != prefixSeparator)) {
        query = str.mid(next).trimmed();

        const QString keywordStr = str.left(sepAt).trimmed();
        keywords = keywordStr.split(keywordSeparator);
    } else {
        query = str.trimmed();
    }

    return SearchQuery(query, keywords);
}

QString SearchQuery::toString() const
{
    if (m_keywords.isEmpty())
        return m_query;
    else
        return m_keywordPrefix + m_query;
}

bool SearchQuery::isEmpty() const
{
    return m_query.isEmpty() && m_keywords.isEmpty();
}

QStringList SearchQuery::keywords() const
{
    return m_keywords;
}

void SearchQuery::setKeywords(const QStringList &list)
{
    if (list.isEmpty())
        return;

    m_keywords = list;
    m_keywordPrefix = list.join(keywordSeparator) + prefixSeparator;
}

bool SearchQuery::hasKeywords() const
{
    return !m_keywords.isEmpty();
}

bool SearchQuery::hasKeyword(const QString &keyword) const
{
    // Temporary workaround for #333
    // TODO: Remove once #167 is implemented
    for (const QString &kw : m_keywords) {
        if (keyword.startsWith(kw, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool SearchQuery::hasKeywords(const QStringList &keywords) const
{
    for (const QString &keyword : keywords) {
        if (m_keywords.contains(keyword))
            return true;
    }
    return false;
}

int SearchQuery::keywordPrefixSize() const
{
    return m_keywordPrefix.size();
}

QString SearchQuery::query() const
{
    return m_query;
}

void SearchQuery::setQuery(const QString &str)
{
    m_query = str;
}

QDataStream &operator<<(QDataStream &out, const Zeal::Registry::SearchQuery &query)
{
    out << query.toString();
    return out;
}

QDataStream &operator>>(QDataStream &in, Zeal::Registry::SearchQuery &query)
{
    QString str;
    in >> str;
    query = SearchQuery::fromString(str);
    return in;
}
