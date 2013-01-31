#ifndef ZEALSEARCHRESULT_H
#define ZEALSEARCHRESULT_H

#include <QString>

class ZealSearchResult
{
public:
    ZealSearchResult(const QString& name_, const QString& parentName_, const QString& path_, const QString& docset_, const QString& query_);
    const QString& getName() const { return name; };
    const QString& getParentName() const { return parentName; };
    const QString& getPath() const { return path; };
    const QString& getDocsetName() const { return docset; };
    bool operator<(const ZealSearchResult& r) const {
        if (name.toLower().startsWith(query.toLower()) > r.name.toLower().startsWith(r.query.toLower())) {
            // return results that are prefixed with query first
            return true;
        } else if (name.toLower().startsWith(query.toLower()) < r.name.toLower().startsWith(r.query.toLower())) {
            return false;
        } else {
            return name.toLower() < r.name.toLower() ||
                (name.toLower() == r.name.toLower()
                    && parentName.toLower() < r.parentName.toLower());
        }
    }
private:
    QString name;
    QString parentName;
    QString path;
    QString docset;
    QString query;
};

#endif // ZEALSEARCHRESULT_H
