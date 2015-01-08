#include "zealsearchresult.h"

ZealSearchResult::ZealSearchResult()
{
}

ZealSearchResult::ZealSearchResult(const QString &name, const QString &parentName,
                                   const QString &path, const QString &docset,
                                   const QString &query) :
    m_name(name),
    m_parentName(parentName),
    m_path(path),
    m_docset(docset),
    m_query(query)
{
}

QString ZealSearchResult::name() const
{
    return m_name;
}

QString ZealSearchResult::parentName() const
{
    return m_parentName;
}

QString ZealSearchResult::path() const
{
    return m_path;
}

QString ZealSearchResult::docsetName() const
{
    return m_docset;
}

bool ZealSearchResult::operator<(const ZealSearchResult &r) const
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
