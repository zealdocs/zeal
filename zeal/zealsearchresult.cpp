#include "zealsearchresult.h"

ZealSearchResult::ZealSearchResult()
{}

ZealSearchResult::ZealSearchResult(const QString &name_,
                                   const QString &parentName_,
                                   const QString &path_,
                                   const QString &docset_,
                                   const QString &query_)
    : name(name_)
    , parentName(parentName_)
    , path(path_)
    , docset(docset_)
    , query(query_)
{}

bool ZealSearchResult::operator<(const ZealSearchResult &r) const
{
    const bool lhsStartsWithQuery = name.startsWith(query, Qt::CaseInsensitive);
    const bool rhsStartsWithQuery = r.name.startsWith(query, Qt::CaseInsensitive);

    if (lhsStartsWithQuery != rhsStartsWithQuery) {
        return lhsStartsWithQuery > rhsStartsWithQuery;
    }

    const int namesCmp = QString::compare(name, r.name, Qt::CaseInsensitive);
    if (namesCmp) {
        return namesCmp < 0;
    }

    return QString::compare(parentName, r.parentName, Qt::CaseInsensitive) < 0;
}
