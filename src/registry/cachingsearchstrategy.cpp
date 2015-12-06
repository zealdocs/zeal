#include "cachingsearchstrategy.h"
#include "docset.h"

#include "searchresult.h"
#include "searchquery.h"

using namespace Zeal;

CachingSearchStrategy::CachingSearchStrategy(std::unique_ptr<DocsetSearchStrategy> strategy)
    : m_search(std::move(strategy)),
      m_cache(CachingSearchStrategy::CacheSize)
{
}

QList<SearchResult> CachingSearchStrategy::search(const QString &query, CancellationToken token)
{
    QString candidate = getCacheEntry(query);
    if (!candidate.isEmpty()) {
        return searchWithCache(query, candidate);
    } else {
        return searchWithoutCache(query, token);
    }
}

bool CachingSearchStrategy::validResult(
        const QString &query, SearchResult previousResult,
        SearchResult &result)
{
    return m_search->validResult(query, previousResult, result);
}

QString CachingSearchStrategy::getCacheEntry(const QString &query) const
{
    const SearchQuery searchQuery = SearchQuery::fromString(query);

    for (int i = searchQuery.query().size(); i > 0; i--) {
        QString candidate = searchQuery.query().mid(0, i);

        /// Use the cache only if the cache entry contains all results.
        /// Also handle cases where prefix is a docset query `std:`.
        if (m_cache.contains(candidate)
                && m_cache[candidate]->size() < Docset::MaxDocsetResultsCount
                && SearchQuery::fromString(candidate).query().size() == i)
            return candidate;
    }
    return QString();
}

QList<SearchResult> CachingSearchStrategy::searchWithCache(const QString &query, const QString &prefix)
{
    QList<SearchResult> *prefixResults = m_cache[prefix];
    QList<SearchResult> results;
    QListIterator<SearchResult> prefixResultsIterator(*prefixResults);
    while (prefixResultsIterator.hasNext()) {
        SearchResult previousResult = prefixResultsIterator.next();
        SearchResult result;
        if (validResult(query, previousResult, result))
            results.append(result);
    }

    m_cache.insert(query, new QList<SearchResult>(results));
    return results;
}

QList<SearchResult> CachingSearchStrategy::searchWithoutCache(const QString &query, CancellationToken token)
{
    QList<SearchResult> results = m_search->search(query, token);

    /// Only cache the results if they are not partial.
    if (!token.isCancelled())
        m_cache.insert(query, new QList<SearchResult>(results));
    return results;
}
