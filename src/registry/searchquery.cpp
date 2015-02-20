#include "searchquery.h"

using namespace Zeal;

namespace {
const char prefixSeparator = ':';
const char keywordSeparator = ',';
}

SearchQuery::SearchQuery()
{
}

SearchQuery::SearchQuery(const QString &query, const QStringList &keywords) :
    m_query(query),
    m_keywords(keywords),
    m_keywordPrefix(keywords.join(keywordSeparator))
{
}

SearchQuery SearchQuery::fromString(const QString &str)
{
    const int sepAt = str.indexOf(prefixSeparator);
    const int next = sepAt + 1;

    QString query;
    QStringList keywords;
    if (sepAt >= 1 && (next >= str.size() || str.at(next) != prefixSeparator)) {
        query = str.midRef(next).toString().trimmed();

        const QString keywordStr = str.leftRef(sepAt).toString().trimmed();
        keywords = keywordStr.split(keywordSeparator);
    } else {
        query = str.trimmed();
    }

    return SearchQuery(query, keywords);
}

QString SearchQuery::toString() const
{
    return m_keywords.join(keywordSeparator) + prefixSeparator + m_query;
}

bool SearchQuery::hasKeywords() const
{
    return !m_keywords.isEmpty();
}

bool SearchQuery::hasKeyword(const QString &keyword) const
{
    for (const QString &kw : m_keywords) {
        if (keyword.contains(kw, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

int SearchQuery::keywordPrefixSize() const
{
    return m_keywordPrefix.size();
}

QString SearchQuery::sanitizedQuery() const
{
    QString q = m_query;
    q.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    q.replace(QStringLiteral("_"), QStringLiteral("\\_"));
    q.replace(QStringLiteral("%"), QStringLiteral("\\%"));
    q.replace(QStringLiteral("'"), QStringLiteral("''"));
    return q;
}

QString SearchQuery::query() const
{
    return m_query;
}
