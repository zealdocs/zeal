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
#include <QRegularExpression>

using namespace Zeal::Registry;

namespace {
const char prefixSeparator = ':';
const char keywordSeparator = ',';
const QRegularExpression RegexpType("\\bt:(?P<type>\\w*)");
const QRegularExpression RegexpDocset("\\bd:(?P<docset>\\w*)");
}

SearchQuery::SearchQuery()
{
}

SearchQuery::SearchQuery(const QString &query, const QStringList &keywords) :
    m_query(query)
{
    setKeywords(keywords);
}

SearchQuery::SearchQuery(const QString &query, const QString &type, const QStringList &keywords) :
    m_query(query), m_type(type)
{
    setKeywords(keywords);
}

SearchQuery SearchQuery::fromString(const QString &str)
{
    auto query = str; // To edit the string
    QStringList keywords;
    auto match_type = RegexpType.match(str);
    auto match_docset = RegexpDocset.match(str);
    if(match_type.hasMatch()) {
        query = query.remove(RegexpType);
    }
    if(match_docset.hasMatch()) {
        keywords << match_docset.captured("docset");
        query = query.remove(RegexpDocset);
    }

    return SearchQuery(query.trimmed(), match_type.captured("type"), keywords);
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

QString SearchQuery::type() const
{
    return m_type;
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
