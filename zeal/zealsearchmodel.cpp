#include <iostream>
#include <numeric>
using namespace std;

#include <QIcon>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "zealsearchmodel.h"
#include "zealdocsetsregistry.h"

ZealSearchModel::ZealSearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
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
    if ((role != Qt::DisplayRole && role != Qt::DecorationRole) || !index.isValid())
        return QVariant();

    auto item = static_cast<ZealSearchResult*>(index.internalPointer());

    if(role == Qt::DecorationRole) {
        if(index.column() == 0) {
            return QVariant(docsets->icon(item->getDocsetName()));
        }
        return QVariant();
    }

    if(index.column() == 0) {
        if(!item->getParentName().isEmpty()) {
            return QVariant(QString("%1 (%2)").arg(item->getName(), item->getParentName()));
        } else {
            return QVariant(item->getName());
        }

    } else if (index.column() == 1) {
        return QVariant(docsets->dir(item->getDocsetName()).absoluteFilePath(item->getPath()));
    }
    return QVariant();
}

QVariant ZealSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

QModelIndex ZealSearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        if(dataList.count() <= row) return QModelIndex();
        auto &item = dataList.at(row);

        if(column == 0 || column == 1) {
            return createIndex(row, column, (void*)&item);
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
    dataList = docsets->getQueryResults();
    emit queryCompleted();
}
