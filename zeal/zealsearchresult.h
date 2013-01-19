#ifndef ZEALSEARCHRESULT_H
#define ZEALSEARCHRESULT_H

#include <QString>

class ZealSearchResult
{
public:
    ZealSearchResult(const QString& name_, const QString& parentName_, const QString& path_, const QString& docset_);
    const QString& getName() const { return name; };
    const QString& getParentName() const { return parentName; };
    const QString& getPath() const { return path; };
    const QString& getDocsetName() const { return docset; };
private:
    QString name;
    QString parentName;
    QString path;
    QString docset;
};

#endif // ZEALSEARCHRESULT_H
