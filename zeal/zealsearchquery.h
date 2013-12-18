#ifndef ZEALSEARCHQUERY_H
#define ZEALSEARCHQUERY_H

class ZealSearchQuery
{
public:
    ZealSearchQuery(const QString& coreQuery);

    static const char DOCSET_FILTER_SEPARATOR = ':';

    QString getDocsetFilter();
    QString getSanitizedQuery();
    QString getCoreQuery();

private:
    QString docsetFilter;
    QString coreQuery;
};

#endif // ZEALSEARCHQUERY_H
