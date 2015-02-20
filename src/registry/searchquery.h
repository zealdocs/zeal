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

    /// Returns true if there's a docset filter for the given query
    bool hasKeywords() const;

    /// Returns true if the docset prefix match the ones given on query
    bool hasKeyword(const QString &keyword) const;

    /// Returns the docset filter raw size for the given query
    int keywordPrefixSize() const;

    /// Returns the core query, sanitized for use in SQL queries
    QString sanitizedQuery() const;

    /// Returns the query with any docset prefixes removed.
    QString query() const;

private:
    QString m_query;
    QStringList m_keywords;
    QString m_keywordPrefix;
};

} // namespace Zeal

#endif // SEARCHQUERY_H
