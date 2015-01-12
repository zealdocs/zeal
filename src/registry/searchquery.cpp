#include "searchquery.h"

using namespace Zeal;

namespace {
const char DOCSET_FILTER_SEPARATOR = ':';
const char MULTIPLE_DOCSET_SEPARATOR = ',';
}

SearchQuery::SearchQuery(const QString &rawQuery)
{
    const int sepAt = rawQuery.indexOf(DOCSET_FILTER_SEPARATOR);
    const int next = sepAt + 1;

    if (sepAt >= 1 && (next >= rawQuery.size() || rawQuery.at(next) != DOCSET_FILTER_SEPARATOR)) {
        m_rawDocsetFilter = rawQuery.leftRef(sepAt).toString().trimmed();
        m_coreQuery = rawQuery.midRef(next).toString().trimmed();
        m_docsetFilters = m_rawDocsetFilter.split(MULTIPLE_DOCSET_SEPARATOR);
    } else {
        m_rawDocsetFilter.clear();
        m_coreQuery = rawQuery.trimmed();
        m_docsetFilters.clear();
    }
}

bool SearchQuery::hasDocsetFilter() const
{
    return !m_rawDocsetFilter.isEmpty();
}

bool SearchQuery::docsetPrefixMatch(const QString &docsetPrefix) const
{
    for (const QString &docsetPrefixFilter : m_docsetFilters) {
        if (docsetPrefix.contains(docsetPrefixFilter, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

int SearchQuery::docsetFilterSize() const
{
    return m_rawDocsetFilter.size();
}

QString SearchQuery::sanitizedQuery() const
{
    QString q = m_coreQuery;
    q.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    q.replace(QStringLiteral("_"), QStringLiteral("\\_"));
    q.replace(QStringLiteral("%"), QStringLiteral("\\%"));
    q.replace(QStringLiteral("'"), QStringLiteral("''"));
    return q;
}

QString SearchQuery::coreQuery() const
{
    return m_coreQuery;
}
