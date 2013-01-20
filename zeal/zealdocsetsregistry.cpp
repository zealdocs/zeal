#include <QThread>
#include <QVariant>
#include <QtSql/QSqlQuery>

#include "zealdocsetsregistry.h"
#include "zealsearchresult.h"

ZealDocsetsRegistry* ZealDocsetsRegistry::m_Instance;

ZealDocsetsRegistry* docsets = ZealDocsetsRegistry::instance();

void ZealDocsetsRegistry::addDocset(const QString& path) {
    auto dir = QDir(path);
    auto db = QSqlDatabase::addDatabase("QSQLITE", dir.dirName());
    db.setDatabaseName(dir.filePath("index.sqlite"));
    db.open();
    dbs.insert(dir.dirName(), db);
    dirs.insert(dir.dirName(), dir);
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
        auto qstr = QString("select name, parent, path from things where name "
                            "like '%1%' order by lower(name) asc, path asc limit 40").arg(query);
        auto q = db(name).exec(qstr);

        while(q.next()) {
            QString parentName;
            if(!q.value(1).isNull()) {
                auto qp = db(name).exec(QString("select name from things where id = %1").arg(q.value(1).toInt()));
                qp.next();
                parentName = qp.value(0).toString();
            }
            results.append(ZealSearchResult(q.value(0).toString(), parentName, q.value(2).toString(), name));
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
