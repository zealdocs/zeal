#include "docsetregistry.h"

#include "searchquery.h"
#include "searchresult.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>
#include <QVariant>

using namespace Zeal;

DocsetRegistry::DocsetRegistry(QObject *parent) :
    QObject(parent)
{
    /// FIXME: Only search should be performed in a separate thread
    QThread *thread = new QThread(this);
    moveToThread(thread);
    thread->start();
}

void DocsetRegistry::init(const QString &path)
{
    blockSignals(true);

    clear();
    addDocsetsFromFolder(path);

    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.cd(QStringLiteral("docsets")))
        addDocsetsFromFolder(appDir);

    blockSignals(false);
    emit reset();
}

int DocsetRegistry::count() const
{
    return m_docsets.count();
}

bool DocsetRegistry::contains(const QString &name) const
{
    return m_docsets.contains(name);
}

QStringList DocsetRegistry::names() const
{
    return m_docsets.keys();
}

void DocsetRegistry::remove(const QString &name)
{
    emit docsetAboutToBeRemoved(name);
    delete m_docsets.take(name);
    emit docsetRemoved(name);
}

Docset *DocsetRegistry::docset(const QString &name) const
{
    return m_docsets[name];
}

Docset *DocsetRegistry::docset(int index) const
{
    if (index < 0 || index >= m_docsets.size())
        return nullptr;
    return (m_docsets.cbegin() + index).value();
}

QList<Docset *> DocsetRegistry::docsets() const
{
    return m_docsets.values();
}

void DocsetRegistry::addDocset(const QString &path)
{
    Docset *docset = new Docset(path);

    /// TODO: Emit error
    if (!docset->isValid()) {
        delete docset;
        return;
    }

    const QString name = docset->name();

    if (m_docsets.contains(name))
        remove(name);

    m_docsets[name] = docset;
    emit docsetAdded(name);
}

void DocsetRegistry::runQuery(const QString &query)
{
    m_lastQuery += 1;
    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query),
                              Q_ARG(int, m_lastQuery));
}

void DocsetRegistry::invalidateQueries()
{
    m_lastQuery += 1;
}

void DocsetRegistry::_runQuery(const QString &rawQuery, int queryNum)
{
    // If some other queries pending, ignore this one.
    if (queryNum != m_lastQuery)
        return;

    QList<SearchResult> results;
    const SearchQuery query(rawQuery);

    const QString preparedQuery = query.sanitizedQuery();

    for (const Docset * const docset : docsets()) {
        // Filter out this docset as the names don't match the docset prefix
        if (query.hasDocsetFilter() && !query.docsetPrefixMatch(docset->prefix))
            continue;

        QString queryStr;
        QList<QList<QVariant>> found;
        bool withSubStrings = false;
        // %.%1% for long Django docset values like django.utils.http
        // %::%1% for long C++ docset values like std::set
        // %/%1% for long Go docset values like archive/tar
        QString subNames = QStringLiteral(" or %1 like '%.%2%' escape '\\'");
        subNames += QStringLiteral(" or %1 like '%::%2%' escape '\\'");
        subNames += QStringLiteral(" or %1 like '%/%2%' escape '\\'");
        while (found.size() < 100) {
            QString curQuery = preparedQuery;
            QString notQuery; // don't return the same result twice
            if (withSubStrings) {
                // if less than 100 found starting with query, search all substrings
                curQuery = "%" + preparedQuery;
                // don't return 'starting with' results twice
                if (docset->type() == Docset::Type::ZDash)
                    notQuery = QString(" and not (ztokenname like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("ztokenname", preparedQuery));
                else
                    notQuery = QString(" and not (t.name like '%1%' escape '\\' %2) ").arg(preparedQuery, subNames.arg("t.name", preparedQuery));
            }
            int cols = 3;
            if (docset->type() == Docset::Type::Dash) {
                queryStr = QString("SELECT t.name, null, t.path FROM searchIndex t WHERE (t.name "
                                   "LIKE '%1%' escape '\\' %3)  %2 ORDER BY length(t.name), lower(t.name) ASC, t.path ASC LIMIT 100")
                        .arg(curQuery, notQuery, subNames.arg("t.name", curQuery));
            } else if (docset->type() == Docset::Type::ZDash) {
                cols = 4;
                queryStr = QString("SELECT ztokenname, null, zpath, zanchor FROM ztoken "
                                   "JOIN ztokenmetainformation on ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                   "JOIN zfilepath on ztokenmetainformation.zfile = zfilepath.z_pk WHERE (ztokenname "
                                   "LIKE '%1%' escape '\\' %3) %2 ORDER BY length(ztokenname), lower(ztokenname) ASC, zpath ASC, "
                                   "zanchor ASC LIMIT 100").arg(curQuery, notQuery,
                                                                subNames.arg("ztokenname", curQuery));
            }

            QSqlQuery query(queryStr, docset->database());
            while (query.next()) {
                QList<QVariant> values;
                for (int i = 0; i < cols; ++i)
                    values.append(query.value(i));
                found.append(values);
            }

            if (withSubStrings)
                break;
            withSubStrings = true;  // try again searching for substrings
        }

        for (const QList<QVariant> &row : found) {
            QString parentName;
            if (!row[1].isNull())
                parentName = row[1].toString();

            QString path = row[2].toString();
            // FIXME: refactoring to use common code in ZealListModel and DocsetRegistry
            if (docset->type() == Docset::Type::ZDash)
                path += "#" + row[3].toString();

            QString itemName = row[0].toString();
            normalizeName(itemName, parentName, row[1].toString());
            results.append(SearchResult(itemName, parentName, path, docset->name(),
                                        preparedQuery));
        }
    }
    std::sort(results.begin(), results.end());
    if (queryNum != m_lastQuery)
        return; // some other queries pending - ignore this one

    m_queryResults = results;
    emit queryCompleted();
}

void DocsetRegistry::normalizeName(QString &itemName, QString &parentName,
                                   const QString &initialParent)
{
    QRegExp matchMethodName("^([^\\(]+)(?:\\(.*\\))?$");
    if (matchMethodName.indexIn(itemName) != -1)
        itemName = matchMethodName.cap(1);

    const QStringList separators = {QStringLiteral("."), QStringLiteral("::"), QStringLiteral("/")};
    for (const QString &sep : separators) {
        if (itemName.indexOf(sep) != -1 && itemName.indexOf(sep) != 0 && initialParent.isNull()) {
            const QStringList splitted = itemName.split(sep);
            itemName = splitted.at(splitted.size()-1);
            parentName = splitted.at(splitted.size()-2);
        }
    }
}

const QList<SearchResult> &DocsetRegistry::queryResults()
{
    return m_queryResults;
}

QList<SearchResult> DocsetRegistry::relatedLinks(const QString &name, const QString &path)
{
    const Docset * const docset = m_docsets[name];

    QList<SearchResult> results;

    // Get the url without the #anchor.
    QUrl url(path);
    url.setFragment(QString());

    // Prepare the query to look up all pages with the same url.
    QString queryStr;
    if (docset->type() == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT name, type, path FROM searchIndex WHERE path LIKE \"%1%%\"");
    } else if (docset->type() == Docset::Type::ZDash) {
        queryStr = QStringLiteral("SELECT ztoken.ztokenname, ztokentype.ztypename, zfilepath.zpath, ztokenmetainformation.zanchor "
                                  "FROM ztoken "
                                  "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk "
                                  "WHERE zfilepath.zpath = \"%1\"");
    }

    QSqlQuery query(queryStr.arg(url.toString()), docset->database());

    while (query.next()) {
        QString sectionName = query.value(0).toString();
        QString sectionPath = query.value(2).toString();
        QString parentName;
        if (docset->type() == Docset::Type::ZDash) {
            sectionPath.append("#");
            sectionPath.append(query.value(3).toString());
        }

        normalizeName(sectionName, parentName);

        results.append(SearchResult(sectionName, QString(), sectionPath, name, QString()));
    }

    return results;
}

// Recursively finds and adds all docsets in a given directory.
void DocsetRegistry::addDocsetsFromFolder(const QDir &folder)
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

void DocsetRegistry::clear()
{
    for (const QString &name : m_docsets.keys())
        delete m_docsets.take(name);
}
