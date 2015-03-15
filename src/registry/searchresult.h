#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <QString>

namespace Zeal {

class Docset;

struct SearchResult
{
    QString name;
    QString parentName;
    QString type;

    Docset *docset;

    QString path;

    /// TODO: Remove
    QString query;

    bool operator<(const SearchResult &r) const;
};

} // namespace Zeal

#endif // SEARCHRESULT_H
