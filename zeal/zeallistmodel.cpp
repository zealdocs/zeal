#include "zeallistmodel.h"
#include "zealdocsetsregistry.h"

#include <QtSql/QSqlQuery>

#include <iostream>
#include <string>

using namespace std;

ZealListModel::ZealListModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    strings = new QSet<QString>;
    modulesCounts = new QHash<QString, int>;
}

Qt::ItemFlags ZealListModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    return QAbstractItemModel::flags(index);
}

const QHash<QString, int> ZealListModel::getModulesCounts() const
{
    if(!modulesCounts->count()) {
        for(auto name : docsets->names()) {
            auto db = docsets->db(name);
            auto q = db.exec("select count(*) from things where type='module'");
            q.next();
            (*modulesCounts)[name] = q.value(0).toInt();
        }
    }
    return *modulesCounts;
}

QModelIndex ZealListModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        if(row >= docsets->count()) return QModelIndex();
        if(column == 0) {
            return createIndex(row, column, (void*)getString(docsets->names().at(row)));
        } else if(column == 1) {
            return createIndex(row, column, (void*)getString(docsets->dir(docsets->names().at(row)).absoluteFilePath("index.html")));
        }
        return QModelIndex();
    } else {
        if(i2s(parent)->indexOf('/') == -1) {
            if(column == 0) {
                return createIndex(row, column, (void*)getString(*i2s(parent)+"/Modules"));
            }
        } else if(i2s(parent)->endsWith("/Modules")) {
            auto name = i2s(parent)->split("/").at(0);
            if(row >= getModulesCounts()[name]) return QModelIndex();
            auto db = docsets->db(name);
            if(column == 0) {
                auto q = db.exec("select name from things where type='module' limit 1 offset " + QString().setNum(row));
                q.next();
                return createIndex(row, column,
                    (void*)getString(QString("%1/Modules/%2").arg(name, q.value(0).toString())));
            } else if(column == 1) {
                auto q = db.exec("select path from things where type='module' limit 1 offset " + QString().setNum(row));
                q.next();
                return createIndex(row, column, (void*)getString(docsets->dir(name).absoluteFilePath(q.value(0).toString())));
            }
        }
        return QModelIndex();
    }
}

QVariant ZealListModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    if(index.column() == 0)
        return QVariant(i2s(index)->split('/').last());
    else {
        return QVariant(*i2s(index));
    }
}

QVariant ZealListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

int ZealListModel::rowCount(const QModelIndex &parent) const
{
    if(parent.column() > 0 && !i2s(parent)->endsWith("/Modules")) return 0;
    if(!parent.isValid()) {
        // root
        return docsets->count();
    } else {
        auto parentStr = i2s(parent);
        if(parentStr->indexOf("/") == -1) {
            // docset - one enty - Modules
            return 1;
        } else if(parentStr->endsWith("/Modules")) {
            // modules count
            return getModulesCounts()[parentStr->split('/').at(0)];
        }
        // module - no sub items
        return 0;
    }
}

const QString *ZealListModel::i2s(const QModelIndex &index) const
{
    return static_cast<const QString*>(index.internalPointer());
}

QModelIndex ZealListModel::parent(const QModelIndex &child) const
{
    if(child.isValid() && i2s(child)->endsWith("/Modules")) {
        return createIndex(0, 0, (void*)getString(i2s(child)->split('/')[0]));
    } else if(child.isValid() && i2s(child)->indexOf("/Modules/") != -1) {
        return createIndex(0, 0, (void*)getString(i2s(child)->split('/')[0] + "/Modules"));
    } else {
        return QModelIndex();
    }
}

int ZealListModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid() || i2s(parent)->endsWith("/Modules"))
        return 2;
    else
        return 1;
}
