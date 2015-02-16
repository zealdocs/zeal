#ifndef DOCSETREGISTRY_H
#define DOCSETREGISTRY_H

#include "docset.h"
#include "searchresult.h"

#include <QMap>

class QThread;

namespace Zeal {

class DocsetRegistry : public QObject
{
    Q_OBJECT
public:
    explicit DocsetRegistry(QObject *parent = nullptr);
    ~DocsetRegistry() override;

    void init(const QString &path);

    int count() const;
    bool contains(const QString &name) const;
    QStringList names() const;
    void remove(const QString &name);

    Docset *docset(const QString &name) const;
    Docset *docset(int index) const;

    QString prepareQuery(const QString &rawQuery);
    void search(const QString &query);
    const QList<SearchResult> &queryResults();
    QList<Docset *> docsets() const;

public slots:
    void addDocset(const QString &path);

signals:
    void docsetAdded(const QString &name);
    void docsetAboutToBeRemoved(const QString &name);
    void docsetRemoved(const QString &name);
    void queryCompleted();

private slots:
    void _addDocset(const QString &path);
    void _runQuery(const QString &rawQuery, int queryNum);

private:
    void addDocsetsFromFolder(const QString &path);

    QThread *m_thread = nullptr;
    QMap<QString, Docset *> m_docsets;
    QList<SearchResult> m_queryResults;
    int m_lastQuery = -1;
};

} // namespace Zeal

#endif // DOCSETREGISTRY_H
