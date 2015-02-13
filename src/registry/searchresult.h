#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <QString>

namespace Zeal {

class Docset;

class SearchResult
{
public:
    SearchResult(const QString &name, const QString &parentName, Docset *docset,
                 const QString &path, const QString &query);

    QString name() const;
    QString parentName() const;

    Docset *docset() const;

    QString path() const;

    bool operator<(const SearchResult &r) const;

private:
    QString m_name;
    QString m_parentName;
    Docset *m_docset = nullptr;
    QString m_path;
    QString m_query;
};

} // namespace Zeal

#endif // SEARCHRESULT_H
