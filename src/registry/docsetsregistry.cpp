#include "docsetsregistry.h"

#include "searchquery.h"
#include "searchresult.h"

#include <QCoreApplication>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>
#include <QVariant>

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

bool DocsetsRegistry::contains(const QString &name) const
{
    return m_docs.contains(name);
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

DocsetsRegistry::DocsetsRegistry()
{
    /// FIXME: Only search should be performed in a separate thread
    auto thread = new QThread(this);
    moveToThread(thread);
    thread->start();
}

QList<Docset> DocsetsRegistry::docsets()
{
    return m_docs.values();
}

void DocsetsRegistry::addDocset(const QString &path)
{
    QDir dir(path);
    QString name = dir.dirName().replace(QStringLiteral(".docset"), QString());

    Docset entry;

    QDir contentsDir(dir.filePath("Contents"));
    entry.info = DocsetInfo::fromPlist(contentsDir.absoluteFilePath("Info.plist"));

    if (entry.info.family == "cheatsheet")
        name = QString("%1_cheats").arg(name);
    entry.name = name;

    QString dashFile = QDir(contentsDir.filePath("Resources")).filePath("docSet.dsidx");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(dashFile);
    db.open();
    auto q = db.exec("select name from sqlite_master where type='table'");

    QStringList tables;
    while (q.next())
        tables.append(q.value(0).toString());

    entry.type = tables.contains("searchIndex") ? Docset::Type::Dash : Docset::Type::ZDash;

    dir.cd("Contents");
    dir.cd("Resources");
    dir.cd("Documents");

    if (m_docs.contains(name))
        remove(name);

    entry.prefix = entry.info.bundleName.isEmpty() ? name : entry.info.bundleName;
    entry.db = db;
    entry.documentPath = dir.absolutePath();

    // Read metadata
    entry.metadata = DocsetMetadata::fromFile(path + QStringLiteral("/meta.json"));
    m_docs[name] = entry;
}

const Docset &DocsetsRegistry::entry(const QString &name)
{
    return m_docs[name];
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

    for (const Docset &docset : docsets()) {
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
                if (docset.type == Docset::Type::ZDash)
                    notQuery = QString(" and not (ztokenname like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("ztokenname", preparedQuery));
                else
                    notQuery = QString(" and not (t.name like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("t.name", preparedQuery));
            }
            int cols = 3;
            if (docset.type == Docset::Type::Dash) {
                qstr = QString("select t.name, null, t.path from searchIndex t where (t.name "
                               "like '%1%' escape '\\' %3)  %2 order by length(t.name), lower(t.name) asc, t.path asc limit 100").arg(curQuery, notQuery, subNames.arg("t.name", curQuery));
            } else if (docset.type == Docset::Type::ZDash) {
                cols = 4;
                qstr = QString("select ztokenname, null, zpath, zanchor from ztoken "
                               "join ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                               "join zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk where (ztokenname "
                               "like '%1%' escape '\\' %3) %2 order by length(ztokenname), lower(ztokenname) asc, zpath asc, "
                               "zanchor asc limit 100").arg(curQuery, notQuery,
                                                            subNames.arg("ztokenname", curQuery));
            }
            q = docset.db.exec(qstr);
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
            if (docset.type == Docset::Type::ZDash)
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
    Docset entry = m_docs[name];

    // Prepare the query to look up all pages with the same url.
    QString query;
    if (entry.type == Docset::Type::Dash) {
        query = QString("SELECT name, type, path FROM searchIndex WHERE path LIKE \"%1%%\"").arg(pageUrl);
    } else if (entry.type == Docset::Type::ZDash) {
        query = QString("SELECT ztoken.ztokenname, ztokentype.ztypename, zfilepath.zpath, ztokenmetainformation.zanchor "
                        "FROM ztoken "
                        "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                        "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                        "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk "
                        "WHERE zfilepath.zpath = \"%1\"").arg(pageUrl);
    }

    QSqlQuery result = entry.db.exec(query);
    while (result.next()) {
        QString sectionName = result.value(0).toString();
        QString sectionPath = result.value(2).toString();
        QString parentName;
        if (entry.type == Docset::Type::ZDash) {
            sectionPath.append("#");
            sectionPath.append(result.value(3).toString());
        }

        normalizeName(sectionName, parentName);

        results.append(SearchResult(sectionName, QString(), sectionPath, name, QString()));
    }

    return results;
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

void DocsetsRegistry::initialiseDocsets(const QString &path)
{
    clear();
    addDocsetsFromFolder(path);
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.cd("docsets"))
        addDocsetsFromFolder(appDir);
}
