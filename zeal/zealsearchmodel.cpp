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
    counts = new QHash<QString, int>;
    strings = new QSet<QString>;
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
        QSqlDatabase db;
        int found = 0, drow = 0;
        QString name;
        for(QString name_ : docsets->names()) {
            db = docsets->db(name_);
            found += getCounts()[name_];
            if(found > row) {
                name = name_;
                break;
            }
            // not enough rows - decrement row
            drow -= getCounts()[name_];
        }
        if(found <= row) return QModelIndex();
        if(column == 0) {
            auto q = db.exec(QString("select name, parent from things where name like '%1%' order by name asc, path asc limit 1 offset ").arg(query)+QString().setNum(row+drow));
            q.next();
            if(!q.value(1).isNull()) {
                auto qp = db.exec(QString("select name from things where id = %1").arg(q.value(1).toInt()));
                qp.next();
                return createIndex(row, column, (void*)getString(
                    QString("%1 (%2)").arg(q.value(0).toString(), qp.value(0).toString())));
            } else {
                return createIndex(row, column, (void*)getString(q.value(0).toString()));
            }

        } else if (column == 1) {
            auto q = db.exec(QString("select path from things where name like '%1%' order by name asc, path asc limit 1 offset ").arg(query)+QString().setNum(row+drow));
            q.next();
            return createIndex(row, column, (void*)getString(docsets->dir(name).absoluteFilePath(q.value(0).toString())));
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
        return accumulate(getCounts().begin(), getCounts().end(), 0);
    }
    return 0;
}

int ZealSearchModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

void ZealSearchModel::setQuery(const QString &q) {
    query = q;
    counts->clear();
    strings->clear();
}

const QHash<QString, int> ZealSearchModel::getCounts() const
{
    if(counts->empty()) {
        for(auto name : docsets->names()) {
            auto db = docsets->db(name);
            auto q = db.exec(QString("select count(name) from things where name like '%1%'").arg(query));
            q.next();
            (*counts)[name] = min(40, q.value(0).toInt());
        }
    }
    return *counts;
}
