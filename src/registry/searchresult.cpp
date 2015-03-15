#include "searchresult.h"

using namespace Zeal;

bool SearchResult::operator<(const SearchResult &r) const
{
    const bool lhsStartsWithQuery = name.startsWith(query, Qt::CaseInsensitive);
    const bool rhsStartsWithQuery = r.name.startsWith(query, Qt::CaseInsensitive);

    if (lhsStartsWithQuery != rhsStartsWithQuery)
        return lhsStartsWithQuery > rhsStartsWithQuery;

    const int namesCmp = QString::compare(name, r.name, Qt::CaseInsensitive);
    if (namesCmp)
        return namesCmp < 0;

    return QString::compare(parentName, r.parentName, Qt::CaseInsensitive) < 0;
}
