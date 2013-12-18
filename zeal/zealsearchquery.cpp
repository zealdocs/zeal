#include <QString>
#include <QStringList>

#include "zealsearchquery.h"

// Creates a search query from a string
//
// Examples:
//   "android:setTypeFa" #=> docsetFilter = "android", coreQuery = "setTypeFa"
//   "noprefix"          #=> docsetFilter = "", coreQuery = "noprefix"
//   ":find"             #=> docsetFilter = "", coreQuery = ":find"
ZealSearchQuery::ZealSearchQuery(const QString &rawQuery)
{
    this->coreQuery = rawQuery;
    this->docsetFilter = "";

    if(rawQuery.indexOf(ZealSearchQuery::DOCSET_FILTER_SEPARATOR) >= 1) {
        QStringList partitioned = rawQuery.split(ZealSearchQuery::DOCSET_FILTER_SEPARATOR);
        QString prefix = partitioned[0];
        this->coreQuery = partitioned[1];
        this->docsetFilter = prefix.trimmed();
    }

    this->coreQuery = this->coreQuery.trimmed();
}


// Returns the docset filter for the given query.
QString ZealSearchQuery::getDocsetFilter()
{
    return this->docsetFilter;
}

// Returns the core query, sanitized for use in SQL queries
QString ZealSearchQuery::getSanitizedQuery()
{
    QString q = getCoreQuery();
    q.replace("\\", "\\\\");
    q.replace("_", "\\_");
    q.replace("%", "\\%");
    q.replace("'", "''");
    return q;
}

// Returns the query with any docset prefixes removed.
QString ZealSearchQuery::getCoreQuery()
{
    return this->coreQuery;
}
