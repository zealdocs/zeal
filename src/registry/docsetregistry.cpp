#include "docsetregistry.h"

#include "searchquery.h"
#include "searchresult.h"

#include <QDir>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>
#include <QVariant>

using namespace Zeal;

DocsetRegistry::DocsetRegistry(QObject *parent) :
    QObject(parent),
    m_thread(new QThread(this))
{
    /// FIXME: Only search should be performed in a separate thread
    moveToThread(m_thread);
    m_thread->start();
}

DocsetRegistry::~DocsetRegistry()
{
    m_thread->exit();
    m_thread->wait();
}

void DocsetRegistry::init(const QString &path)
{
    for (const QString &name : m_docsets.keys())
        remove(name);

    addDocsetsFromFolder(path);
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
    QMetaObject::invokeMethod(this, "_addDocset", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, path));
}

void DocsetRegistry::_addDocset(const QString &path)
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

void DocsetRegistry::search(const QString &query)
{
    m_lastQuery += 1;

    // Only invalidate queries
    if (query.isEmpty())
        return;

    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query),
                              Q_ARG(int, m_lastQuery));
}

void DocsetRegistry::_runQuery(const QString &rawQuery, int queryNum)
{
    // If some other queries pending, ignore this one.
    if (queryNum != m_lastQuery)
        return;

    QList<SearchResult> results;
    const SearchQuery query = SearchQuery::fromString(rawQuery);

    const QString preparedQuery = query.sanitizedQuery();

    for (Docset *docset : docsets()) {
        // Filter out this docset as the names don't match the docset prefix
        if (query.hasKeywords() && !query.hasKeyword(docset->prefix))
            continue;

        QString queryStr;
        QList<QList<QVariant>> found;
        bool withSubStrings = false;
        // %.%1% for long Django docset values like django.utils.http
        // %::%1% for long C++ docset values like std::set
        // %/%1% for long Go docset values like archive/tar
        QString subNames = QStringLiteral(" or %1 like '%.%2%' escape '\\'");
        subNames += QLatin1String(" or %1 like '%::%2%' escape '\\'");
        subNames += QLatin1String(" or %1 like '%/%2%' escape '\\'");
        while (found.size() < 100) {
            QString curQuery = preparedQuery;
            QString notQuery; // don't return the same result twice
            if (withSubStrings) {
                // if less than 100 found starting with query, search all substrings
                curQuery = QLatin1Char('%') + preparedQuery;
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
            /// FIXME: refactoring to use common code in ZealListModel and DocsetRegistry
            if (docset->type() == Docset::Type::ZDash)
                path += QLatin1Char('#') + row[3].toString();

            QString itemName = row[0].toString();
            //Docset::normalizeName(itemName, parentName);
            /// TODO: Third should be type
            results.append(SearchResult{itemName, parentName, QString(), docset, path, preparedQuery});
        }
    }
    std::sort(results.begin(), results.end());
    if (queryNum != m_lastQuery)
        return; // some other queries pending - ignore this one

    m_queryResults = results;
    emit queryCompleted();
}

const QList<SearchResult> &DocsetRegistry::queryResults()
{
    return m_queryResults;
}

// Recursively finds and adds all docsets in a given directory.
void DocsetRegistry::addDocsetsFromFolder(const QString &path)
{
    const QDir dir(path);
    for (const QFileInfo &subdir : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == QLatin1String("docset"))
            addDocset(subdir.absoluteFilePath());
        else
            addDocsetsFromFolder(subdir.absoluteFilePath());
    }
}
