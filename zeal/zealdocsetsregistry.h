#ifndef ZEALDOCSETSREGISTRY_H
#define ZEALDOCSETSREGISTRY_H

#include <QDir>
#include <QIcon>
#include <QMap>
#include <QMutex>
#include <QSqlDatabase>

#include "zealsearchresult.h"
#include "zealdocsetinfo.h"
#include "zealdocsetmetadata.h"

enum DocsetType {
    ZEAL, DASH, ZDASH
};

class ZealDocsetsRegistry : public QObject
{
    Q_OBJECT
public:
    struct DocsetEntry {
        QString name;
        QString prefix;
        QSqlDatabase db;
        QDir dir;
        DocsetType type;
        ZealDocsetMetadata metadata;
        ZealDocsetInfo info;
    };

    static ZealDocsetsRegistry *instance();

    int count() const;
    QStringList names() const;
    void remove(const QString &name);
    void clear();

    QSqlDatabase &db(const QString &name);
    const QDir &dir(const QString &name);
    const ZealDocsetMetadata &meta(const QString &name);
    QIcon icon(const QString &docsetName) const;
    DocsetType type(const QString &name) const;

    DocsetEntry *entry(const QString &name);
    // Returns the list of links available in a given webpage.
    // Scans the list of related links for a given page. This lets you view the methods of a given object.
    QList<ZealSearchResult> relatedLinks(const QString &name, const QString &path);
    QString prepareQuery(const QString &rawQuery);
    void runQuery(const QString &query);
    void invalidateQueries();
    const QList<ZealSearchResult> &queryResults();
    QList<DocsetEntry> docsets();

    QString docsetsDir() const;
    void initialiseDocsets();

public slots:
    void addDocset(const QString &path);

signals:
    void queryCompleted();

private slots:
    void _runQuery(const QString &query, int queryNum);

private:
    ZealDocsetsRegistry();
    Q_DISABLE_COPY(ZealDocsetsRegistry)

    void addDocsetsFromFolder(const QDir &folder);
    void normalizeName(QString &itemName, QString &parentName, const QString &initialParent);

    static ZealDocsetsRegistry *m_instance;
    QMap<QString, DocsetEntry> m_docs;
    QList<ZealSearchResult> m_queryResults;
    int m_lastQuery = -1;
};

#endif // ZEALDOCSETSREGISTRY_H
