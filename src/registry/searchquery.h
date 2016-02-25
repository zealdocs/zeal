/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef SEARCHQUERY_H
#define SEARCHQUERY_H

#include <QStringList>

namespace Zeal {

/**
 * @short The search query model.
 */
class SearchQuery
{
public:
    explicit SearchQuery();
    explicit SearchQuery(const QString &query, const QStringList &keywords = QStringList());

    /// Creates a search query from a string. Single separator will be
    /// used to contstruct docset filter, but separator repeated twice
    /// will be left inside coreQuery part since double semicolon is
    /// used inside qualified symbol names in popular programming
    /// languages (c++, ruby, perl, etc.).
    ///
    /// Examples:
    ///   "android:setTypeFa" #=> docsetFilters = ["android"], coreQuery = "setTypeFa"
    ///   "noprefix"          #=> docsetFilters = [], coreQuery = "noprefix"
    ///   ":find"             #=> docsetFilters = [], coreQuery = ":find"
    ///   "std::string"       #=> docsetFilters = [], coreQuery = "std::string"
    ///   "c++:std::string"   #=> docsetFilters = ["c++"], coreQuery = "std::string"
    ///
    /// Multiple docsets are supported using the ',' character:
    ///   "java,android:setTypeFa #=> docsetFilters = ["java", "android"], coreQuery = "setTypeFa"

    static SearchQuery fromString(const QString &str);

    QString toString() const;

    bool isEmpty() const;

    QStringList keywords() const;
    void setKeywords(const QStringList &list);

    /// Returns true if there's a docset filter for the given query
    bool hasKeywords() const;

    /// Returns true if the docset prefix match the ones given on query
    bool hasKeyword(const QString &keyword) const;
    bool hasKeywords(const QStringList &keywords) const;

    /// Returns the docset filter raw size for the given query
    int keywordPrefixSize() const;

    QString query() const;
    void setQuery(const QString &str);

    /// Returns the core query, sanitized for use in SQL queries
    QString sanitizedQuery() const;
    QString sanitizedQuerySubseq() const;

private:
    QString m_query;
    QStringList m_keywords;
    QString m_keywordPrefix;
};

QDataStream &operator<<(QDataStream &out, const SearchQuery &query);
QDataStream &operator>>(QDataStream &in, SearchQuery &query);

} // namespace Zeal

#endif // SEARCHQUERY_H
