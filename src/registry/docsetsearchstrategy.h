#ifndef DOCSETSEARCHSTRATEGY_H
#define DOCSETSEARCHSTRATEGY_H

#include "cancellationtoken.h"

#include <QString>
#include <QList>

namespace Zeal {

class SearchResult;

/// This class contains search related methods for docsets.
class DocsetSearchStrategy
{
public:
    DocsetSearchStrategy();
    virtual QList<SearchResult> search(const QString &query, CancellationToken token) = 0;

    /// Used to filter out cached results.
    virtual bool validResult(const QString &query, SearchResult previousResult,
                             SearchResult &result) = 0;
};

}

#endif // DOCSETSEARCHSTRATEGY_H
