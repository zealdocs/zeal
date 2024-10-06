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

#ifndef ZEAL_REGISTRY_SEARCHQUERY_H
#define ZEAL_REGISTRY_SEARCHQUERY_H

#include <QDataStream>
#include <QStringList>

namespace Zeal {
namespace Registry {

/**
 * @short The search query model.
 */
class SearchQuery
{
public:
    explicit SearchQuery() = default;
    explicit SearchQuery(QString query, const QStringList &keywords = QStringList());

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

    /// Returns true if one the query contains one of the @c keywords.
    bool hasKeywords(const QStringList &keywords) const;

    /// Returns the docset filter raw size for the given query
    int keywordPrefixSize() const;

    QString query() const;
    void setQuery(const QString &str);

private:
    QString m_query;
    QStringList m_keywords;
    QString m_keywordPrefix;
};

} // namespace Registry
} // namespace Zeal

QDataStream &operator<<(QDataStream &out, const Zeal::Registry::SearchQuery &query);
QDataStream &operator>>(QDataStream &in, Zeal::Registry::SearchQuery &query);

#endif // ZEAL_REGISTRY_SEARCHQUERY_H
