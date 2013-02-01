#include <QThread>
#include <QVariant>
#include <QtSql/QSqlQuery>

#include "zealdocsetsregistry.h"
#include "zealsearchresult.h"

ZealDocsetsRegistry* ZealDocsetsRegistry::m_Instance;

ZealDocsetsRegistry* docsets = ZealDocsetsRegistry::instance();

void ZealDocsetsRegistry::addDocset(const QString& path) {
    auto dir = QDir(path);
    auto name = dir.dirName().replace(".docset", "");
    auto db = QSqlDatabase::addDatabase("QSQLITE", name);
    if(QFile::exists(dir.filePath("index.sqlite"))) {
        db.setDatabaseName(dir.filePath("index.sqlite"));
        db.open();
        types.insert(name, ZEAL);
    } else {
        auto dashFile = QDir(QDir(dir.filePath("Contents")).filePath("Resources")).filePath("docSet.dsidx");
        db.setDatabaseName(dashFile);
        db.open();
        auto q = db.exec("select name from sqlite_master where type='table'");
        QStringList tables;
        while(q.next()) {
            tables.append(q.value(0).toString());
        }
        if(tables.contains("searchIndex")) {
            types.insert(name, DASH);
        } else {
            types.insert(name, ZDASH);
        }
    }
    dbs.insert(name, db);
    dirs.insert(name, dir);
}

ZealDocsetsRegistry::ZealDocsetsRegistry() {
    auto thread = new QThread(this);
    moveToThread(thread);
    thread->start();
}

void ZealDocsetsRegistry::runQuery(const QString& query)
{
    lastQuery += 1;
    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query), Q_ARG(int, lastQuery));
}

void ZealDocsetsRegistry::_runQuery(const QString& query, int queryNum)
{
    if(queryNum != lastQuery) return; // some other queries pending - ignore this one

    QList<ZealSearchResult> results;
    for(auto name : names()) {
        QString qstr;
        QSqlQuery q;
        bool found = false;
        bool withSubStrings = false;
        while(!found) {
            auto curQuery = query;
            if(withSubStrings) {
                // if nothing found starting with query, search all substrings
                curQuery = "%"+query;
            }
            if(types[name] == ZEAL) {
                qstr = QString("select name, parent, path from things where name "
                               "like '%1%' order by lower(name) asc, path asc limit 40").arg(curQuery);
            } else if(types[name] == DASH) {
                qstr = QString("select name, null, path from searchIndex where name "
                               "like '%1%' order by lower(name) asc, path asc limit 40").arg(curQuery);
            } else if(types[name] == ZDASH) {
                qstr = QString("select ztokenname, null, zpath, zanchor from ztoken "
                                "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk where ztokenname "
                               // %.%1% for long Django docset values like django.utils.http
                               // (Might be not appropriate for other docsets, but I don't have any on hand to test)
                               "like '%1%' or ztokenname like '%.%1%' order by lower(ztokenname) asc, zpath asc, zanchor asc limit 40").arg(curQuery);
            }
            q = db(name).exec(qstr);
            if(q.next()) { found = true; }
            else {
                if(withSubStrings) break;
                withSubStrings = true;  // try again searching for substrings
            }
        }
        if(!found) continue;
        do {
            QString parentName;
            if(!q.value(1).isNull()) {
                auto qp = db(name).exec(QString("select name from things where id = %1").arg(q.value(1).toInt()));
                qp.next();
                parentName = qp.value(0).toString();
            }
            auto path = q.value(2).toString();
            // FIXME: refactoring to use common code in ZealListModel and ZealDocsetsRegistry
            // TODO: parent name, splitting by '.', as in ZealDocsetsRegistry
            if(types[name] == DASH || types[name] == ZDASH) {
                path = QDir(QDir(QDir("Contents").filePath("Resources")).filePath("Documents")).filePath(path);
            }
            if(types[name] == ZDASH) {
                path += "#" + q.value(3).toString();
            }
            auto itemName = q.value(0).toString();
            if(itemName.indexOf('.') != -1 && itemName.indexOf('.') != 0 && q.value(1).isNull()) {
                auto splitted = itemName.split(".");
                itemName = splitted.at(splitted.size()-1);
                parentName = splitted.at(splitted.size()-2);
            }
            results.append(ZealSearchResult(itemName, parentName, path, name, query));
        } while (q.next());
    }
    qSort(results);
    if(queryNum != lastQuery) return; // some other queries pending - ignore this one

    queryResults = results;
    emit queryCompleted();
}

const QList<ZealSearchResult>& ZealDocsetsRegistry::getQueryResults()
{
    return queryResults;
}
