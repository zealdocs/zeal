#ifndef ZEALSEARCHRESULT_H
#define ZEALSEARCHRESULT_H

#include <QString>

class ZealSearchResult
{
public:
    ZealSearchResult();
    ZealSearchResult(const QString &name, const QString &parentName, const QString &path,
                     const QString &docset, const QString &query);

    QString name() const;
    QString parentName() const;
    QString path() const;
    QString docsetName() const;

    bool operator<(const ZealSearchResult &r) const;

private:
    QString m_name;
    QString m_parentName;
    QString m_path;
    QString m_docset;
    QString m_query;
};

#endif // ZEALSEARCHRESULT_H
