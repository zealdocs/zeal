#include "zealsearchresult.h"

ZealSearchResult::ZealSearchResult(const QString& name_, const QString& parentName_, const QString& path_, const QString& docset_, const QString& query_)
    : name(name_), parentName(parentName_), path(path_), docset(docset_), query(query_)
{
}
