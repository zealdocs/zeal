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
    if (m_keywords.isEmpty())
        return m_query;
    else
        return m_keywords.join(keywordSeparator) + prefixSeparator + m_query;
}

bool SearchQuery::isEmpty() const
{
    return m_query.isEmpty() && m_keywords.isEmpty();
}

QStringList SearchQuery::keywords() const
{
    return m_keywords;
}

void SearchQuery::setKeywords(const QStringList &list)
{
    m_keywords = list;
}

bool SearchQuery::hasKeywords() const
{
    return !m_keywords.isEmpty();
}

bool SearchQuery::hasKeyword(const QString &keyword) const
{
    // Temporary workaround for #333
    /// TODO: Remove once #167 is implemented
    for (const QString &kw : m_keywords) {
        if (keyword.startsWith(kw, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool SearchQuery::hasKeywords(const QStringList &keywords) const
{
    for (const QString &keyword : keywords) {
        if (m_keywords.contains(keyword))
            return true;
    }
    return false;
}

int SearchQuery::keywordPrefixSize() const
{
    return m_keywordPrefix.size();
}

QString SearchQuery::query() const
{
    return m_query;
}

void SearchQuery::setQuery(const QString &str)
{
    m_query = str;
}

QString SearchQuery::sanitizedQuery() const
{
    QString q = m_query;
    q.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    q.replace(QLatin1String("_"), QLatin1String("\\_"));
    q.replace(QLatin1String("%"), QLatin1String("\\%"));
    q.replace(QLatin1String("'"), QLatin1String("''"));
    return q;
}

QDataStream &Zeal::operator<<(QDataStream &out, const SearchQuery &query)
{
    out << query.toString();
    return out;
}

QDataStream &Zeal::operator>>(QDataStream &in, SearchQuery &query)
{
    QString str;
    in >> str;
    query = SearchQuery::fromString(str);
    return in;
}
