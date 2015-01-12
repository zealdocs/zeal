#ifndef ZEALSEARCHMODEL_H
#define ZEALSEARCHMODEL_H

#include "searchresult.h"

#include <QAbstractItemModel>

namespace Zeal {

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SearchModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
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

#endif // ZEALSEARCHMODEL_H
