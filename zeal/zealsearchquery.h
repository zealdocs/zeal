#ifndef ZEALSEARCHQUERY_H
#define ZEALSEARCHQUERY_H

#include <QStringList>

/**
 * @short The search query model.
 */
class ZealSearchQuery
{
public:
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
    explicit ZealSearchQuery(const QString &coreQuery);

    /// Returns true if there's a docset filter for the given query
    bool hasDocsetFilter() const;

    /// Returns true if the docset prefix match the ones given on query
    bool docsetPrefixMatch(const QString &docsetPrefix) const;

    /// Returns the docset filter raw size for the given query
    int docsetFilterSize() const;

    /// Returns the core query, sanitized for use in SQL queries
    QString sanitizedQuery() const;

    /// Returns the query with any docset prefixes removed.
    QString coreQuery() const;

private:
    QString m_rawDocsetFilter;
    QStringList m_docsetFilters;
    QString m_coreQuery;
};

#endif // ZEALSEARCHQUERY_H
