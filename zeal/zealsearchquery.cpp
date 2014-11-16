#include <QString>
#include <QStringList>
#include "zealsearchquery.h"

ZealSearchQuery::ZealSearchQuery(const QString &rawQuery)
{
    const int sepAt = rawQuery.indexOf(DOCSET_FILTER_SEPARATOR);
    const int next  = sepAt + 1;

    if (sepAt >= 1
        && (next >= rawQuery.size()
            || rawQuery.at(next) != DOCSET_FILTER_SEPARATOR)) {
        rawDocsetFilter = rawQuery.leftRef(sepAt).toString().trimmed();
        coreQuery = rawQuery.midRef(next).toString().trimmed();
        docsetFilters = rawDocsetFilter.split(MULTIPLE_DOCSET_SEPARATOR);
    } else {
        rawDocsetFilter = "";
        coreQuery = rawQuery.trimmed();
        docsetFilters = QStringList();
    }
}

bool ZealSearchQuery::hasDocsetFilter()
{
    return !rawDocsetFilter.isEmpty();
}

bool ZealSearchQuery::docsetPrefixMatch(QString docsetPrefix)
{
    for (const QString docsetPrefixFilter : docsetFilters) {
        if (docsetPrefix.contains(docsetPrefixFilter, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

int ZealSearchQuery::getDocsetFilterSize()
{
    return rawDocsetFilter.size();
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
    return coreQuery;
}
