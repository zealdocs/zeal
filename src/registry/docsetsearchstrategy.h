#ifndef DOCSETSEARCHSTRATEGY_H
#define DOCSETSEARCHSTRATEGY_H

#include <QString>
#include <QList>

namespace Zeal {

struct CancellationToken;
struct SearchResult;

/// This class contains search related methods for docsets.
class DocsetSearchStrategy
{
public:
    DocsetSearchStrategy();
    virtual QList<SearchResult> search(const QString &query, const CancellationToken &token) const = 0;
};

}

#endif // DOCSETSEARCHSTRATEGY_H
