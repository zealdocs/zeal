#ifndef ZEALSEARCHQUERY_H
#define ZEALSEARCHQUERY_H

class ZealSearchQuery
{
    QString rawQuery;

public:
    ZealSearchQuery();
    ZealSearchQuery(const QString& rawQuery);

    static const char DOCSET_FILTER_SEPARATOR = '|';

    QString getDocsetFilter();
    QString getSanitizedQuery();
    QString getCoreQuery();
};

#endif // ZEALSEARCHQUERY_H
