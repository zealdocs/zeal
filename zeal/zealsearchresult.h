#ifndef ZEALSEARCHRESULT_H
#define ZEALSEARCHRESULT_H

#include <QString>

class ZealSearchResult
{
public:

    ZealSearchResult();

    ZealSearchResult(const QString &name_,
                     const QString &parentName_,
                     const QString &path_,
                     const QString &docset_,
                     const QString &query_);

    QString getName() const { return name; }

    QString getParentName() const { return parentName; }

    QString getPath() const { return path; }

    QString getDocsetName() const { return docset; }

    bool operator<(const ZealSearchResult& r) const;

private:
    QString name;
    QString parentName;
    QString path;
    QString docset;
    QString query;
};

#endif // ZEALSEARCHRESULT_H
