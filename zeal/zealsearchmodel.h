#ifndef ZEALSEARCHMODEL_H
#define ZEALSEARCHMODEL_H

#include <QAbstractItemModel>
#include "zealsearchresult.h"

class ZealSearchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ZealSearchModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    void setQuery(const QString& q);
    
signals:
    void queryCompleted();

public slots:
    void onQueryCompleted();

private:
    QString query;
    QList<ZealSearchResult> dataList;
    void populateData();
};

#endif // ZEALSEARCHMODEL_H
