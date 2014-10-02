#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QtSql/QSqlQuery>
#include <QStandardPaths>

#include "zealdocsetsregistry.h"
#include "zealsearchresult.h"
#include "zealsearchquery.h"

ZealDocsetsRegistry* ZealDocsetsRegistry::m_Instance;

ZealDocsetsRegistry* docsets = ZealDocsetsRegistry::instance();

void ZealDocsetsRegistry::addDocset(const QString& path) {
    auto dir = QDir(path);
    auto name = dir.dirName().replace(".docset", "");
    QSqlDatabase db;
    docsetEntry entry;

    if(QFile::exists(dir.filePath("index.sqlite"))) {
        db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(dir.filePath("index.sqlite"));
        db.open();
        entry.name = name;
        entry.prefix = name;
        entry.type = ZEAL;
    } else {
        QDir contentsDir(dir.filePath("Contents"));
        entry.info.readDocset(contentsDir.absoluteFilePath("Info.plist"));

        if (entry.info.family == "cheatsheet") {
            name = QString("%1_cheats").arg(name);
        }
        entry.name = name;

        auto dashFile = QDir(contentsDir.filePath("Resources")).filePath("docSet.dsidx");
        db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(dashFile);
        db.open();
        auto q = db.exec("select name from sqlite_master where type='table'");
        QStringList tables;
        while(q.next()) {
            tables.append(q.value(0).toString());
        }
        if(tables.contains("searchIndex")) {
            entry.type = DASH;
        } else {
            entry.type = ZDASH;
        }

        dir.cd("Contents");
        dir.cd("Resources");
        dir.cd("Documents");
    }

    if(docs.contains(name)){
        remove(name);
    }
    entry.prefix = entry.info.bundleName.isEmpty()
            ? name
            : entry.info.bundleName;
    entry.db = db;
    entry.dir = dir;

    // Read metadata
    ZealDocsetMetadata meta;
    meta.read(path+"/meta.json");
    entry.metadata = meta;
    docs[name] = entry;
}

ZealDocsetsRegistry::ZealDocsetsRegistry() :
    settings("Zeal", "Zeal")
{
    lastQuery = -1;
    auto thread = new QThread(this);
    moveToThread(thread);
    thread->start();
}

QList<ZealDocsetsRegistry::docsetEntry> ZealDocsetsRegistry::docsets()
{
    return this->docs.values();
}

// Sets the docset prefixes.
void ZealDocsetsRegistry::setPrefixes(QHash<QString, QVariant> docsetPrefixes)
{
    QHashIterator<QString, QVariant> it(docsetPrefixes);
    while (it.hasNext()) {
        it.next();
        QString docsetName = it.key();
        QString docsetPrefix = it.value().toString();
        if (this->docs.contains(docsetName)) {
            docsetEntry docset = this->docs.value(docsetName);
            docset.prefix = docsetPrefix;
        }
    }
}

ZealDocsetsRegistry::docsetEntry *ZealDocsetsRegistry::getEntry(const QString& name)
{
    return &docs[name];
}

void ZealDocsetsRegistry::runQuery(const QString& query)
{
    lastQuery += 1;
    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query), Q_ARG(int, lastQuery));
}

void ZealDocsetsRegistry::invalidateQueries()
{
    lastQuery += 1;
}

void ZealDocsetsRegistry::_runQuery(const QString& rawQuery, int queryNum)
{
    if(queryNum != lastQuery) return; // some other queries pending - ignore this one

    QList<ZealSearchResult> results;
    ZealSearchQuery query(rawQuery);

    QString docsetPrefix = query.getDocsetFilter();
    QString preparedQuery = query.getSanitizedQuery();
    bool hasPrefixFilter = !docsetPrefix.isEmpty();

    for (const ZealDocsetsRegistry::docsetEntry docset : docsets()) {
        if (hasPrefixFilter && !docset.prefix.contains(docsetPrefix, Qt::CaseInsensitive)) {
            // Filter out this docset as the names don't match the docset prefix
            continue;
        }

        QString qstr;
        QSqlQuery q;
        QList<QList<QVariant> > found;
        bool withSubStrings = false;
        // %.%1% for long Django docset values like django.utils.http
        // %::%1% for long C++ docset values like std::set
        // %/%1% for long Go docset values like archive/tar
        QString subNames = QString(" or %1 like '%.%2%' escape '\\'");
        subNames += QString(" or %1 like '%::%2%' escape '\\'");
        subNames += QString(" or %1 like '%/%2%' escape '\\'");
        while(found.size() < 100) {
            auto curQuery = preparedQuery;
            QString notQuery; // don't return the same result twice
            QString parentQuery;
            if(withSubStrings) {
                // if less than 100 found starting with query, search all substrings
                curQuery = "%"+preparedQuery;
                // don't return 'starting with' results twice
                if(docset.type == ZDASH) {
                    notQuery = QString(" and not (ztokenname like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("ztokenname", preparedQuery));
                } else {
                    if(docset.type == ZEAL) {
                        notQuery = QString(" and not (t.name like '%1%' escape '\\') ").arg(preparedQuery);
                        parentQuery = QString(" or t2.name like '%1%' escape '\\' ").arg(preparedQuery);
                    } else { // DASH
                        notQuery = QString(" and not (t.name like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("t.name", preparedQuery));
                    }
                }
            }
            int cols = 3;
            if(docset.type == ZEAL) {
                qstr = QString("select t.name, t2.name, t.path from things t left join things t2 on t2.id=t.parent where "
                               "(t.name like '%1%' escape '\\'  %3) %2 order by length(t.name), lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, parentQuery);

            } else if(docset.type == DASH) {
                qstr = QString("select t.name, null, t.path from searchIndex t where (t.name "
                               "like '%1%' escape '\\' %3)  %2 order by length(t.name), lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, subNames.arg("t.name", curQuery));
            } else if(docset.type == ZDASH) {
                cols = 4;
                qstr = QString("select ztokenname, null, zpath, zanchor from ztoken "
                                "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk where (ztokenname "

                               "like '%1%' escape '\\' %3) %2 order by length(ztokenname), lower(ztokenname) asc, zpath asc, "
                               "zanchor asc limit 100").arg(curQuery, notQuery, subNames.arg("ztokenname", curQuery));
            }
            q = db(docset.name).exec(qstr);
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
            if(docset.type == ZDASH) {
                path += "#" + row[3].toString();
            }
            auto itemName = row[0].toString();
            normalizeName(itemName, parentName, row[1].toString());
            results.append(ZealSearchResult(itemName, parentName, path, docset.name, preparedQuery));
        }
    }
    qSort(results);
    if(queryNum != lastQuery) return; // some other queries pending - ignore this one

    queryResults = results;
    emit queryCompleted();
}

void ZealDocsetsRegistry::normalizeName(QString &itemName, QString &parentName, QString initialParent)
{
    QRegExp matchMethodName("^([^\\(]+)(?:\\(.*\\))?$");
    if (matchMethodName.indexIn(itemName) != -1) {
        itemName = matchMethodName.cap(1);
    }
    QString separators[] = {".", "::", "/"};
    for(unsigned i = 0; i < sizeof separators / sizeof *separators; ++i) {
        QString sep = separators[i];
        if(itemName.indexOf(sep) != -1 && itemName.indexOf(sep) != 0 && initialParent.isNull()) {
            auto splitted = itemName.split(sep);
            itemName = splitted.at(splitted.size()-1);
            parentName = splitted.at(splitted.size()-2);
        }
    }
}

const QList<ZealSearchResult>& ZealDocsetsRegistry::getQueryResults()
{
    return queryResults;
}

QList<ZealSearchResult> ZealDocsetsRegistry::getRelatedLinks(QString name, QString path)
{
    QList<ZealSearchResult> results;
    // Get the url without the #anchor.
    QUrl mainUrl(path);
    mainUrl.setFragment(NULL);
    QString pageUrl(mainUrl.toString());
    docsetEntry entry = docs[name];

    // Prepare the query to look up all pages with the same url.
    QString query;
    if (entry.type == DASH) {
        query = QString("SELECT name, type, path FROM searchIndex WHERE path LIKE \"%1%%\"").arg(pageUrl);
    } else if (entry.type == ZDASH) {
        query = QString("SELECT ztoken.ztokenname, ztokentype.ztypename, zfilepath.zpath, ztokenmetainformation.zanchor "
                        "FROM ztoken "
                        "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                        "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                        "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk "
                        "WHERE zfilepath.zpath = \"%1\"").arg(pageUrl);
    } else if (entry.type == ZEAL) {
        query = QString("SELECT name type, path FROM things WHERE path LIKE \"%1%%\"").arg(pageUrl);
    }

    QSqlQuery result = entry.db.exec(query);
    while (result.next()) {
        QString sectionName = result.value(0).toString();
        QString sectionPath = result.value(2).toString();
        QString parentName;
        if (entry.type == ZDASH) {
            sectionPath.append("#");
            sectionPath.append(result.value(3).toString());
        }

        normalizeName(sectionName, parentName, "");

        results.append(ZealSearchResult(sectionName, "", sectionPath, name, QString()));
    }

    return results;
}

QString ZealDocsetsRegistry::docsetsDir(){
    if(settings.contains("docsetsDir")) {
        return settings.value("docsetsDir").toString();
    } else {
        auto dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        auto dataDir = QDir(dataLocation);
        if(!dataDir.cd("docsets")) {
            dataDir.mkpath("docsets");
        }
        dataDir.cd("docsets");
        return dataDir.absolutePath();
    }
}

// Recursively finds and adds all docsets in a given directory.
void ZealDocsetsRegistry::addDocsetsFromFolder(QDir folder)
{
    for(QFileInfo subdir : folder.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == "docset") {
            QMetaObject::invokeMethod(this, "addDocset", Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, subdir.absoluteFilePath()));
        } else {
            addDocsetsFromFolder(QDir(subdir.absoluteFilePath()));
        }
    }
}

void ZealDocsetsRegistry::initialiseDocsets()
{
    clear();
    addDocsetsFromFolder(QDir(docsetsDir()));
    QDir appDir( QCoreApplication::applicationDirPath() );
    if(appDir.cd("docsets")){
        addDocsetsFromFolder(appDir);
    }
}
