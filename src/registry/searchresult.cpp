#include "searchresult.h"

using namespace Zeal;

SearchResult::SearchResult()
{
}

SearchResult::SearchResult(const QString &name, const QString &parentName, const QString &path,
                           const QString &docset, const QString &query) :
    m_name(name),
    m_parentName(parentName),
    m_path(path),
    m_docset(docset),
    m_query(query)
{
}

QString SearchResult::name() const
{
    return m_name;
}

QString SearchResult::parentName() const
{
    return m_parentName;
}

QString SearchResult::path() const
{
    return m_path;
}

QString SearchResult::docsetName() const
{
    return m_docset;
}

bool SearchResult::operator<(const SearchResult &r) const
{
    const bool lhsStartsWithQuery = m_name.startsWith(m_query, Qt::CaseInsensitive);
    const bool rhsStartsWithQuery = r.m_name.startsWith(m_query, Qt::CaseInsensitive);

    if (lhsStartsWithQuery != rhsStartsWithQuery)
        return lhsStartsWithQuery > rhsStartsWithQuery;

    const int namesCmp = QString::compare(m_name, r.m_name, Qt::CaseInsensitive);
    if (namesCmp)
        return namesCmp < 0;

    return QString::compare(m_parentName, r.m_parentName, Qt::CaseInsensitive) < 0;
}
