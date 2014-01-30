#include <QString>
#include "zealsearchquery.h"

ZealSearchQuery::ZealSearchQuery(const QString &rawQuery)
{
    const int sepAt = rawQuery.indexOf(DOCSET_FILTER_SEPARATOR);
    const int next  = sepAt + 1;
    if (sepAt >= 1
        && (next >= rawQuery.size()
            || rawQuery.at(next) != DOCSET_FILTER_SEPARATOR)) {
        this->docsetFilter = rawQuery.leftRef(sepAt).toString().trimmed();
        this->coreQuery = rawQuery.midRef(next).toString().trimmed();
    } else {
        this->docsetFilter = "";
        this->coreQuery = rawQuery.trimmed();
    }
}

QString ZealSearchQuery::getDocsetFilter()
{
    return this->docsetFilter;
}

QString ZealSearchQuery::getSanitizedQuery()
{
    QString q = getCoreQuery();
    q.replace("\\", "\\\\");
    q.replace("_", "\\_");
    q.replace("%", "\\%");
    q.replace("'", "''");
    return q;
}

QString ZealSearchQuery::getCoreQuery()
{
    return this->coreQuery;
}
