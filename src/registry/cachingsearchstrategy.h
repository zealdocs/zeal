#ifndef CACHINGSEARCHSTRATEGY_H
#define CACHINGSEARCHSTRATEGY_H

#include "cancellationtoken.h"
#include "docsetsearchstrategy.h"

#include <QCache>
#include <QList>
#include <QString>

#include <memory>

namespace Zeal {

class SearchResult;

/// A search strategy that uses a cache of previous searches.
/// If a search was made for a prefix then then all results
/// must appear in the cache. In this case instead of searching
/// entire docset only cache is searched.
class CachingSearchStrategy : public DocsetSearchStrategy
{
public:
    CachingSearchStrategy(std::unique_ptr<DocsetSearchStrategy> strategy);
    QList<SearchResult> search(const QString &query, CancellationToken token) override;
    bool validResult(const QString &query, SearchResult previousResult,
                     SearchResult &result)override;

private:
    QString getCacheEntry(const QString &query) const;
    QList<SearchResult> searchWithCache(const QString &query, const QString &prefix);
    QList<SearchResult> searchWithoutCache(const QString &query, CancellationToken token);

    const static int CacheSize = 10;

    std::unique_ptr<DocsetSearchStrategy> m_search;
    QCache<QString, QList<SearchResult> > m_cache;
};

}

#endif // CACHINGSEARCHSTRATEGY_H
