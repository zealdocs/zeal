#include "docsetregistry.h"

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
    // Only invalidate queries
    if (query.isEmpty())
        return;

    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query));
}

void DocsetRegistry::_runQuery(const QString &query)
{
    m_queryResults.clear();

    for (Docset *docset : docsets())
        m_queryResults << docset->search(query);

    std::sort(m_queryResults.begin(), m_queryResults.end());

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
