#include "zeallistmodel.h"
#include "zealdocsetsregistry.h"

#include <QIcon>
#include <QtSql/QSqlQuery>
#include <QDomDocument>
#include <QDebug>

#include <iostream>
#include <string>

using namespace std;

ZealListModel::ZealListModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    strings = new QSet<QString>;
    modulesCounts = new QHash<QPair<QString, QString>, int>;
    items = new QHash<QPair<QString, int>, QPair<QString, QString> >;
}

Qt::ItemFlags ZealListModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    return QAbstractItemModel::flags(index);
}

const QHash<QPair<QString, QString>, int> ZealListModel::getModulesCounts() const
{
    if(!modulesCounts->count()) {
        for(auto name : docsets->names()) {
            auto db = docsets->db(name);
            QSqlQuery q;
            if(docsets->type(name) == ZEAL) {
                q = db.exec("select type, count(*) from things group by type");
            } else if(docsets->type(name) == DASH) {
                q = db.exec("select type, count(*) from searchIndex group by type");
            } else if(docsets->type(name) == ZDASH) {
                q = db.exec("select ztokentype, count(*) from ztoken group by ztokentype");
            }
            while(q.next()) {
                if(q.value(1).toInt() < 1500) {
                    QString typeName;
                    if(docsets->type(name) == ZEAL || docsets->type(name) == DASH) {
                        typeName = q.value(0).toString();
                    } else { // ZDASH
                        auto q2 = db.exec(QString("select ztypename from ztokentype where z_pk=%1").arg(q.value(0).toInt()));
                        q2.next();
                        typeName = q2.value(0).toString();
                    }
                    (*modulesCounts)[QPair<QString, QString>(name, typeName)] = q.value(1).toInt();
                }
            }
        }
    }
    return *modulesCounts;
}

const QPair<QString, QString> ZealListModel::getItem(const QString& path, int index) const {
    QPair<QString, int> pair(path, index);
    if(items->find(pair) != items->end()) return (*items)[pair];

    auto docsetName = path.split('/')[0];
    auto type = singularize(path.split('/')[1]);
    auto db = docsets->db(docsetName);
    QSqlQuery q;
    if(docsets->type(docsetName) == ZEAL) {
        q = db.exec(QString("select name, path from things where type='%1' order by name asc").arg(type));
    } else if(docsets->type(docsetName) == DASH) {
        q = db.exec(QString("select name, path from searchIndex where type='%1' order by name asc").arg(type));
    } else { // ZDASH
        q = db.exec(QString("select ztokenname, zpath, zanchor from ztoken "
                            "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                            "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk "
                            "join ztokentype on ztoken.ztokentype = ztokentype.z_pk where ztypename='%1' "
                            "order by ztokenname asc").arg(type));
    }
    int i = 0;
    while(q.next()) {
        QPair<QString, QString> item;
        item.first = q.value(0).toString();
        auto filePath = q.value(1).toString();
        // FIXME: refactoring to use common code in ZealListModel and ZealDocsetsRegistry
        // TODO: parent name, splitting by '.', as in ZealDocsetsRegistry
        if(docsets->type(docsetName) == DASH || docsets->type(docsetName) == ZDASH) {
            filePath = QDir(QDir(QDir("Contents").filePath("Resources")).filePath("Documents")).filePath(filePath);
        }
        if(docsets->type(docsetName) == ZDASH) {
            filePath += "#" + q.value(2).toString();
        }
        item.second = docsets->dir(docsetName).absoluteFilePath(filePath);
        (*items)[QPair<QString, int>(path, i)] = item;
        i += 1;
    }
    return (*items)[pair];
}

QModelIndex ZealListModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        if(row >= docsets->count() || row == -1) return QModelIndex();
        if(column == 0) {
            return createIndex(row, column, (void*)getString(docsets->names().at(row)));
        } else if(column == 1) {
            QDir dir(docsets->dir(docsets->names().at(row)));

            if(QFile(dir.absoluteFilePath("index.html")).exists()) {
                return createIndex(row, column, (void*)getString(dir.absoluteFilePath("index.html")));
            } else {
                // retrieve index file name from Info.plist for Dash docsets
                dir.cd("Contents");
                QFile file(dir.absoluteFilePath("Info.plist"));
                QDomDocument infoplist("infoplist");
                if(!file.open(QIODevice::ReadOnly)) {
                    return createIndex(row, column, (void*)getString(""));
                }
                if(!infoplist.setContent(&file)) {
                    file.close();
                    return createIndex(row, column, (void*)getString(""));
                }
                auto keys = infoplist.elementsByTagName("key");
                for(int i = 0; i < keys.count(); ++i) {
                    auto key = keys.at(i);
                    if(key.firstChild().nodeValue() == "dashIndexFilePath") {
                        auto path = key.nextSibling().firstChild().nodeValue().split("/");
                        auto filename = path.last();
                        path.removeLast();
                        dir.cd("Resources"); dir.cd("Documents");
                        for(auto directory : path) {
                            if(!dir.cd(directory)) {
                                return createIndex(row, column, (void*)getString(""));
                            }
                        }
                        return createIndex(row, column, (void*)getString(dir.absoluteFilePath(filename)));
                    }
                }
                // fallback to index.html
                dir.cd("Resources"); dir.cd("Documents");
                return createIndex(row, column, (void*)getString(dir.absoluteFilePath("index.html")));
            }
        }
        return QModelIndex();
    } else {
        QString docsetName;
        for(auto name : docsets->names()) {
            if(i2s(parent)->startsWith(name+"/")) {
                docsetName = name;
            }
        }
        if(docsetName.isEmpty()) {
            // i2s(parent) == docsetName
            if(column == 0) {
                QList<QString> types;
                for(auto &pair : getModulesCounts().keys()) {
                    if(pair.first == *i2s(parent)) {
                        types.append(pair.second);
                    }
                }
                qSort(types);
                return createIndex(row, column, (void*)getString(*i2s(parent)+"/"+pluralize(types[row])));
            }
        } else {
            auto type = singularize(i2s(parent)->split('/')[1]);
            if(row >= getModulesCounts()[QPair<QString, QString>(docsetName, type)]) return QModelIndex();
            if(column == 0) {
                return createIndex(row, column,
                    (void*)getString(QString("%1/%2/%3").arg(docsetName, pluralize(type), getItem(*i2s(parent), row).first)));
            } else if(column == 1) {
                return createIndex(row, column, (void*)getString(getItem(*i2s(parent), row).second));
            }
        }
        return QModelIndex();
    }
}

QVariant ZealListModel::data(const QModelIndex &index, int role) const
{
    if((role != Qt::DisplayRole && role != Qt::DecorationRole) || !index.isValid())
        return QVariant();
    if(role == Qt::DecorationRole) {
        if(i2s(index)->indexOf('/') == -1) {
            return QVariant(docsets->icon(*i2s(index)));
        } else return QVariant();
    }
    if(index.column() == 0) {
        auto retlist = i2s(index)->split('/');
        QString retval = retlist.last();
        if(i2s(index)->count("/") > 2) {  // name with slashes - trim only "docset/type"
            for (int i = retlist.length() - 2; i > 1; --i) retval = retlist[i] + "/" + retval;
        }
        return QVariant(retval);
    } else {
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
            // docset - show types
            int numTypes = 0;
            auto keys = getModulesCounts().keys();
            for(auto &key : keys) {
                if(parentStr == key.first) {
                    numTypes += 1;
                }
            }
            return numTypes;
        } else if(parentStr->count("/") == 1) { // parent is docset/type
            // type count
            auto type = singularize(parentStr->split("/")[1]);
            return getModulesCounts()[QPair<QString, QString>(parentStr->split('/')[0], type)];
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
    if(child.isValid() && i2s(child)->count("/") == 1) { // docset/type
        return createIndex(0, 0, (void*)getString(i2s(child)->split('/')[0]));
    } else if(child.isValid() && i2s(child)->count("/") >= 2) { // docset/type/item (item can contain slashes)
        return createIndex(0, 0, (void*)getString(i2s(child)->split('/')[0] + "/" + i2s(child)->split('/')[1]));
    } else {
        return QModelIndex();
    }
}

int ZealListModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid() || i2s(parent)->count("/") == 1) // child of docset/type
        return 2;
    else
        return 1;
}

QString ZealListModel::pluralize(const QString& s) {
    QString ret = s;
    if(s.endsWith('s')) {
        return ret+"es";
    } else {
        return ret+"s";
    }
}
QString ZealListModel::singularize(const QString& s) {
    QString ret = s;
    if(ret.endsWith("ses")) {
        ret.chop(2);
    } else {
        ret.chop(1);
    }
    return ret;
}

bool ZealListModel::removeRows(int row, int count, const QModelIndex &parent) {
    if(parent.isValid()) return false;
    beginRemoveRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i) {
        docsets->remove(docsets->names()[row + i]);
    }
    endRemoveRows();
    return true;
}
