#include "docsetsregistry.h"

#include "searchquery.h"
#include "searchresult.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

using namespace Zeal;

DocsetsRegistry *DocsetsRegistry::m_instance;

DocsetsRegistry *DocsetsRegistry::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        mutex.lock();

        if (!m_instance)
            m_instance = new DocsetsRegistry();

        mutex.unlock();
    }

    return m_instance;
}

int DocsetsRegistry::count() const
{
    return m_docs.count();
}

QStringList DocsetsRegistry::names() const
{
    return m_docs.keys();
}

void DocsetsRegistry::remove(const QString &name)
{
    m_docs[name].db.close();
    m_docs.remove(name);
}

void DocsetsRegistry::clear()
{
    for (const QString &key : m_docs.keys())
        remove(key);
}

QSqlDatabase &DocsetsRegistry::db(const QString &name)
{
    Q_ASSERT(m_docs.contains(name));
    return m_docs[name].db;
}

const QDir &DocsetsRegistry::dir(const QString &name)
{
    Q_ASSERT(m_docs.contains(name));
    return m_docs[name].dir;
}

const DocsetMetadata &DocsetsRegistry::meta(const QString &name)
{
    Q_ASSERT(m_docs.contains(name));
    return m_docs[name].metadata;
}

QIcon DocsetsRegistry::icon(const QString &docsetName) const
{
    const DocsetEntry &entry = m_docs[docsetName];
    QString bundleName = entry.info.bundleName;
    bundleName.replace(" ", "_");
    QString identifier = entry.info.bundleIdentifier;
    QIcon icon(entry.dir.absoluteFilePath("favicon.ico"));
    if (icon.availableSizes().isEmpty())
        icon = QIcon(entry.dir.absoluteFilePath("icon.png"));

    if (icon.availableSizes().isEmpty()) {
        icon = QIcon(QString("icons:%1.png").arg(bundleName));

        // Fallback to identifier and docset file name.
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(identifier));
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(docsetName));
    }
    return icon;
}

DocsetType DocsetsRegistry::type(const QString &name) const
{
    Q_ASSERT(m_docs.contains(name));
    return m_docs[name].type;
}

DocsetsRegistry::DocsetsRegistry()
{
    /// FIXME: Only search should be performed in a separate thread
    auto thread = new QThread(this);
    moveToThread(thread);
    thread->start();
}

QList<DocsetsRegistry::DocsetEntry> DocsetsRegistry::docsets()
{
    return m_docs.values();
}

void DocsetsRegistry::addDocset(const QString &path)
{
    QDir dir(path);
    auto name = dir.dirName().replace(QStringLiteral(".docset"), QString());
    QSqlDatabase db;
    DocsetEntry entry;

    if (QFile::exists(dir.filePath("index.sqlite"))) {
        db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(dir.filePath("index.sqlite"));
        db.open();
        entry.name = name;
        entry.prefix = name;
        entry.type = ZEAL;
    } else {
        QDir contentsDir(dir.filePath("Contents"));
        entry.info.readDocset(contentsDir.absoluteFilePath("Info.plist"));

        if (entry.info.family == "cheatsheet")
            name = QString("%1_cheats").arg(name);
        entry.name = name;

        auto dashFile = QDir(contentsDir.filePath("Resources")).filePath("docSet.dsidx");
        db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(dashFile);
        db.open();
        auto q = db.exec("select name from sqlite_master where type='table'");
        QStringList tables;
        while (q.next())
            tables.append(q.value(0).toString());

        if (tables.contains("searchIndex"))
            entry.type = DASH;
        else
            entry.type = ZDASH;

        dir.cd("Contents");
        dir.cd("Resources");
        dir.cd("Documents");
    }

    if (m_docs.contains(name))
        remove(name);

    entry.prefix = entry.info.bundleName.isEmpty()
                   ? name
                   : entry.info.bundleName;
    entry.db = db;
    entry.dir = dir;

    // Read metadata
    DocsetMetadata meta;
    meta.read(path+"/meta.json");
    entry.metadata = meta;
    m_docs[name] = entry;
}

DocsetsRegistry::DocsetEntry *DocsetsRegistry::entry(const QString &name)
{
    return &m_docs[name];
}

void DocsetsRegistry::runQuery(const QString &query)
{
    m_lastQuery += 1;
    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query),
                              Q_ARG(int, m_lastQuery));
}

void DocsetsRegistry::invalidateQueries()
{
    m_lastQuery += 1;
}

void DocsetsRegistry::_runQuery(const QString &rawQuery, int queryNum)
{
    // If some other queries pending, ignore this one.
    if (queryNum != m_lastQuery)
        return;

    QList<SearchResult> results;
    SearchQuery query(rawQuery);

    QString preparedQuery = query.sanitizedQuery();
    bool hasDocsetFilter = query.hasDocsetFilter();

    for (const DocsetsRegistry::DocsetEntry &docset : docsets()) {
        // Filter out this docset as the names don't match the docset prefix
        if (hasDocsetFilter && !query.docsetPrefixMatch(docset.prefix))
            continue;

        QString qstr;
        QSqlQuery q;
        QList<QList<QVariant>> found;
        bool withSubStrings = false;
        // %.%1% for long Django docset values like django.utils.http
        // %::%1% for long C++ docset values like std::set
        // %/%1% for long Go docset values like archive/tar
        QString subNames = QStringLiteral(" or %1 like '%.%2%' escape '\\'");
        subNames += QStringLiteral(" or %1 like '%::%2%' escape '\\'");
        subNames += QStringLiteral(" or %1 like '%/%2%' escape '\\'");
        while (found.size() < 100) {
            auto curQuery = preparedQuery;
            QString notQuery; // don't return the same result twice
            QString parentQuery;
            if (withSubStrings) {
                // if less than 100 found starting with query, search all substrings
                curQuery = "%" + preparedQuery;
                // don't return 'starting with' results twice
                if (docset.type == ZDASH) {
                    notQuery = QString(" and not (ztokenname like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("ztokenname", preparedQuery));
                } else {
                    if (docset.type == ZEAL) {
                        notQuery = QString(" and not (t.name like '%1%' escape '\\') ").arg(preparedQuery);
                        parentQuery = QString(" or t2.name like '%1%' escape '\\' ").arg(preparedQuery);
                    } else { // DASH
                        notQuery = QString(" and not (t.name like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("t.name", preparedQuery));
                    }
                }
            }
            int cols = 3;
            if (docset.type == ZEAL) {
                qstr = QString("select t.name, t2.name, t.path from things t left join things t2 on t2.id=t.parent where "
                               "(t.name like '%1%' escape '\\'  %3) %2 order by length(t.name), lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, parentQuery);

            } else if (docset.type == DASH) {
                qstr = QString("select t.name, null, t.path from searchIndex t where (t.name "
                               "like '%1%' escape '\\' %3)  %2 order by length(t.name), lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, subNames.arg("t.name", curQuery));
            } else if (docset.type == ZDASH) {
                cols = 4;
                qstr = QString("select ztokenname, null, zpath, zanchor from ztoken "
                               "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                               "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk where (ztokenname "
                               "like '%1%' escape '\\' %3) %2 order by length(ztokenname), lower(ztokenname) asc, zpath asc, "
                               "zanchor asc limit 100").arg(curQuery, notQuery,
                                                            subNames.arg("ztokenname", curQuery));
            }
            q = db(docset.name).exec(qstr);
            while (q.next()) {
                QList<QVariant> values;
                for (int i = 0; i < cols; ++i)
                    values.append(q.value(i));
                found.append(values);
            }

            if (withSubStrings)
                break;
            withSubStrings = true;  // try again searching for substrings
        }

        for (const auto &row : found) {
            QString parentName;
            if (!row[1].isNull())
                parentName = row[1].toString();

            auto path = row[2].toString();
            // FIXME: refactoring to use common code in ZealListModel and DocsetsRegistry
            if (docset.type == ZDASH)
                path += "#" + row[3].toString();

            auto itemName = row[0].toString();
            normalizeName(itemName, parentName, row[1].toString());
            results.append(SearchResult(itemName, parentName, path, docset.name,
                                            preparedQuery));
        }
    }
    qSort(results);
    if (queryNum != m_lastQuery)
        return; // some other queries pending - ignore this one

    m_queryResults = results;
    emit queryCompleted();
}

void DocsetsRegistry::normalizeName(QString &itemName, QString &parentName,
                                        const QString &initialParent)
{
    QRegExp matchMethodName("^([^\\(]+)(?:\\(.*\\))?$");
    if (matchMethodName.indexIn(itemName) != -1)
        itemName = matchMethodName.cap(1);

    QString separators[] = {".", "::", "/"};
    for (unsigned i = 0; i < sizeof separators / sizeof *separators; ++i) {
        QString sep = separators[i];
        if (itemName.indexOf(sep) != -1 && itemName.indexOf(sep) != 0 && initialParent.isNull()) {
            auto splitted = itemName.split(sep);
            itemName = splitted.at(splitted.size()-1);
            parentName = splitted.at(splitted.size()-2);
        }
    }
}

const QList<SearchResult> &DocsetsRegistry::queryResults()
{
    return m_queryResults;
}

QList<SearchResult> DocsetsRegistry::relatedLinks(const QString &name, const QString &path)
{
    QList<SearchResult> results;
    // Get the url without the #anchor.
    QUrl mainUrl(path);
    mainUrl.setFragment(NULL);
    QString pageUrl(mainUrl.toString());
    DocsetEntry entry = m_docs[name];

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

        normalizeName(sectionName, parentName);

        results.append(SearchResult(sectionName, QString(), sectionPath, name, QString()));
    }

    return results;
}

QString DocsetsRegistry::docsetsDir() const
{
    const QScopedPointer<const QSettings> settings(new QSettings());
    if (settings->contains("docsetsDir"))
        return settings->value("docsetsDir").toString();

    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!dataDir.cd("docsets"))
        dataDir.mkpath("docsets");
    dataDir.cd("docsets");
    return dataDir.absolutePath();
}

// Recursively finds and adds all docsets in a given directory.
void DocsetsRegistry::addDocsetsFromFolder(const QDir &folder)
{
    for (const QFileInfo &subdir : folder.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == "docset") {
            QMetaObject::invokeMethod(this, "addDocset", Qt::BlockingQueuedConnection,
                                      Q_ARG(QString, subdir.absoluteFilePath()));
        } else {
            addDocsetsFromFolder(QDir(subdir.absoluteFilePath()));
        }
    }
}

void DocsetsRegistry::initialiseDocsets()
{
    clear();
    addDocsetsFromFolder(QDir(docsetsDir()));
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.cd("docsets"))
        addDocsetsFromFolder(appDir);
}
