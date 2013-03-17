#include <QThread>
#include <QVariant>
#include <QDebug>
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

void ZealDocsetsRegistry::_runQuery(const QString& query_, int queryNum)
{
    if(queryNum != lastQuery) return; // some other queries pending - ignore this one

    QList<ZealSearchResult> results;
    QString query = query_;
    query.replace("\\", "\\\\");
    query.replace("_", "\\_");
    query.replace("%", "\\%");
    query.replace("'", "''");
    for(auto name : names()) {
        QString qstr;
        QSqlQuery q;
        QList<QList<QVariant> > found;
        bool withSubStrings = false;
        while(found.size() < 100) {
            auto curQuery = query;
            QString notQuery; // don't return the same result twice
            QString parentQuery;
            if(withSubStrings) {
                // if less than 100 found starting with query, search all substrings
                curQuery = "%"+query;
                if(types[name] == ZDASH) {
                    notQuery = QString(" and not (ztokenname like '%1%' escape '\\' or ztokenname like '%.%1%' escape '\\') ").arg(query);
                } else {
                    notQuery = QString(" and not t.name like '%1%' escape '\\' ").arg(query);
                    if(types[name] == ZEAL) {
                        parentQuery = QString(" or t2.name like '%1%' escape '\\'  ").arg(query);
                    }
                }
            }
            int cols = 3;
            if(types[name] == ZEAL) {
                qstr = QString("select t.name, t2.name, t.path from things t left join things t2 on t2.id=t.parent where "
                               "(t.name like '%1%' escape '\\'  %3) %2 order by lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, parentQuery);

            } else if(types[name] == DASH) {
                qstr = QString("select t.name, null, t.path from searchIndex t where t.name "
                               "like '%1%' escape '\\'  %2 order by lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery);
            } else if(types[name] == ZDASH) {
                cols = 4;
                qstr = QString("select ztokenname, null, zpath, zanchor from ztoken "
                                "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk where (ztokenname "
                               // %.%1% for long Django docset values like django.utils.http
                               // (Might be not appropriate for other docsets, but I don't have any on hand to test)
                               "like '%1%' escape '\\'  or ztokenname like '%.%1%' escape '\\' ) %2 order by lower(ztokenname) asc, zpath asc, "
                               "zanchor asc limit 100").arg(curQuery, notQuery);
            }
            q = db(name).exec(qstr);
            while(q.next()) {
                QList<QVariant> values;
                for(int i = 0; i < cols; ++i) {
                    values.append(q.value(i));
                }
                found.append(values);
            }

            if(withSubStrings) break;
            withSubStrings = true;  // try again searching for substrings
        }
        for(auto &row : found) {
            QString parentName;
            if(!row[1].isNull()) {
                parentName = row[1].toString();
            }
            auto path = row[2].toString();
            // FIXME: refactoring to use common code in ZealListModel and ZealDocsetsRegistry
            if(types[name] == DASH || types[name] == ZDASH) {
                path = QDir(QDir(QDir("Contents").filePath("Resources")).filePath("Documents")).filePath(path);
            }
            if(types[name] == ZDASH) {
                path += "#" + row[3].toString();
            }
            auto itemName = row[0].toString();
            if(itemName.indexOf('.') != -1 && itemName.indexOf('.') != 0 && row[1].isNull()) {
                auto splitted = itemName.split(".");
                itemName = splitted.at(splitted.size()-1);
                parentName = splitted.at(splitted.size()-2);
            }
            results.append(ZealSearchResult(itemName, parentName, path, name, query));
        }
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
