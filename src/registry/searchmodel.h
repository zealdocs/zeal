#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include "searchresult.h"

#include <QAbstractItemModel>

namespace Zeal {

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SearchModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    void setQuery(const QString &q);

signals:
    void queryCompleted();

public slots:
    void onQueryCompleted(const QList<SearchResult> &results);

private:
    QString query;
    QList<SearchResult> dataList;
    void populateData();
};

} // namespace Zeal

#endif // SEARCHMODEL_H
