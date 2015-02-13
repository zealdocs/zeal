#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <QString>

namespace Zeal {

class SearchResult
{
public:
    SearchResult(const QString &name, const QString &parentName, const QString &path,
                 const QString &docset, const QString &query);

    QString name() const;
    QString parentName() const;
    QString path() const;
    QString docsetName() const;

    bool operator<(const SearchResult &r) const;

private:
    QString m_name;
    QString m_parentName;
    QString m_path;
    QString m_docset;
    QString m_query;
};

} // namespace Zeal

#endif // SEARCHRESULT_H
