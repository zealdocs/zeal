// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchquery.h"

#include <utility>

using namespace Zeal::Registry;

namespace {
const char prefixSeparator = ':';
const char keywordSeparator = ',';
} // namespace

SearchQuery::SearchQuery(QString query, const QStringList &keywords)
    : m_query(std::move(query))
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
    if (m_keywords.isEmpty()) {
        return m_query;
    }

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

bool SearchQuery::hasKeywords(const QStringList &keywords) const
{
    for (const QString &keyword : keywords) {
        if (m_keywords.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
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
