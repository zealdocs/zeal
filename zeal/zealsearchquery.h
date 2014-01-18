#ifndef ZEALSEARCHQUERY_H
#define ZEALSEARCHQUERY_H

class QString;

/**
 * @short The search query model.
 */
class ZealSearchQuery
{
public:
    static const char DOCSET_FILTER_SEPARATOR = ':';

    /// Creates a search query from a string. Single separator will be
    /// used to contstruct docset filter, but separator repeated twice
    /// will be left inside coreQuery part since double semicolon is
    /// used inside qualified symbol names in popular programming
    /// languages (c++, ruby, perl, etc.).
    ///
    /// Examples:
    ///   "android:setTypeFa" #=> docsetFilter = "android", coreQuery = "setTypeFa"
    ///   "noprefix"          #=> docsetFilter = "", coreQuery = "noprefix"
    ///   ":find"             #=> docsetFilter = "", coreQuery = ":find"
    ///   "std::string"       #=> docsetFilter = "", coreQuery = "std::string"
    ///   "c++:std::string"   #=> docsetFilter = "c++", coreQuery = "std::string"
    ZealSearchQuery(const QString& coreQuery);

    /// Returns the docset filter for the given query.
    QString getDocsetFilter();

    /// Returns the core query, sanitized for use in SQL queries.
    QString getSanitizedQuery();

    /// Returns the query with any docset prefixes removed.
    QString getCoreQuery();

private:
    QString docsetFilter;
    QString coreQuery;
};

#endif // ZEALSEARCHQUERY_H
