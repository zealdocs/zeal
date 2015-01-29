#ifndef DOCSETSREGISTRY_H
#define DOCSETSREGISTRY_H

#include <QDir>
#include <QIcon>
#include <QMap>
#include <QMutex>
#include <QSqlDatabase>

#include "searchresult.h"
#include "docsetinfo.h"
#include "docsetmetadata.h"

namespace Zeal {

enum DocsetType {
    DASH,
    ZDASH
};

class DocsetsRegistry : public QObject
{
    Q_OBJECT
public:
    struct DocsetEntry {
        QString name;
        QString prefix;
        DocsetType type;
        QString documentPath;
        QSqlDatabase db;
        DocsetMetadata metadata;
        DocsetInfo info;
    };

    static DocsetsRegistry *instance();

    int count() const;
    QStringList names() const;
    void remove(const QString &name);
    void clear();

    QIcon icon(const QString &docsetName) const;
    DocsetType type(const QString &name) const;

    const DocsetEntry &entry(const QString &name);
    // Returns the list of links available in a given webpage.
    // Scans the list of related links for a given page. This lets you view the methods of a given object.
    QList<SearchResult> relatedLinks(const QString &name, const QString &path);
    QString prepareQuery(const QString &rawQuery);
    void runQuery(const QString &query);
    void invalidateQueries();
    const QList<SearchResult> &queryResults();
    QList<DocsetEntry> docsets();

    void initialiseDocsets(const QString &path);

public slots:
    void addDocset(const QString &path);

signals:
    void queryCompleted();

private slots:
    void _runQuery(const QString &query, int queryNum);

private:
    DocsetsRegistry();
    Q_DISABLE_COPY(DocsetsRegistry)

    void addDocsetsFromFolder(const QDir &folder);
    void normalizeName(QString &itemName, QString &parentName,
                       const QString &initialParent = QString());

    static DocsetsRegistry *m_instance;
    QMap<QString, DocsetEntry> m_docs;
    QList<SearchResult> m_queryResults;
    int m_lastQuery = -1;
};

} // namespace Zeal

#endif // DOCSETSREGISTRY_H
