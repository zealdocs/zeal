#include <iostream>
#include <numeric>
using namespace std;

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "zealsearchmodel.h"
#include "zealdocsetsregistry.h"

ZealSearchModel::ZealSearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    strings = new QSet<QString>;
    connect(docsets, &ZealDocsetsRegistry::queryCompleted, this, &ZealSearchModel::onQueryCompleted);
}

Qt::ItemFlags ZealSearchModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    return QAbstractItemModel::flags(index);
}

QVariant ZealSearchModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    return QVariant(*static_cast<QString*>(index.internalPointer()));
}

QVariant ZealSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

QModelIndex ZealSearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        if(dataList.count() <= row) return QModelIndex();
        auto item = dataList.at(row);

        if(column == 0) {
            if(!item.getParentName().isEmpty()) {
                return createIndex(row, column, (void*)getString(
                                       QString("%1 (%2)").arg(item.getName(), item.getParentName())));
            } else {
                return createIndex(row, column, (void*)getString(item.getName()));
            }

        } else if (column == 1) {
            return createIndex(row, column, (void*)getString(docsets->dir(item.getDocsetName()).absoluteFilePath(item.getPath())));
        }
    }
    return QModelIndex();
}

QModelIndex ZealSearchModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int ZealSearchModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return dataList.count();
    }
    return 0;
}

int ZealSearchModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

void ZealSearchModel::setQuery(const QString &q) {
    query = q;
    populateData();
}

void ZealSearchModel::populateData()
{
    docsets->runQuery(query);
}

void ZealSearchModel::onQueryCompleted()
{
    strings->clear();
    dataList = docsets->getQueryResults();
    emit queryCompleted();
}
