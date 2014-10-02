#ifndef ZEALDOCSETSREGISTRY_H
#define ZEALDOCSETSREGISTRY_H

#include <QDebug>
#include <QCoreApplication>
#include <QMutex>
#include <QtSql/QSqlDatabase>
#include <QDir>
#include <QIcon>
#include <QMap>
#include <QSettings>
#include <QJsonObject>

#include "zealsearchresult.h"
#include "zealdocsetinfo.h"
#include "zealdocsetmetadata.h"

typedef enum {ZEAL, DASH, ZDASH} DocSetType;

class ZealDocsetsRegistry : public QObject
{
    Q_OBJECT
public:
    typedef struct {
        QString name;
        QString prefix;
        QSqlDatabase db;
        QDir dir;
        DocSetType type;
        ZealDocsetMetadata metadata;
        ZealDocsetInfo info;
    } docsetEntry;

    static ZealDocsetsRegistry* instance()
    {
        static QMutex mutex;
        if (!m_Instance)
        {
            mutex.lock();

            if (!m_Instance)
                m_Instance = new ZealDocsetsRegistry;

            mutex.unlock();
        }

        return m_Instance;
    }

    int count() const {
        return docs.count();
    }

    QSqlDatabase& db(const QString& name) {
        Q_ASSERT(docs.contains(name));
        return docs[name].db;
    }

    const QDir& dir(const QString& name) {
        Q_ASSERT(docs.contains(name));
        return docs[name].dir;
    }

    const ZealDocsetMetadata& meta(const QString& name){
        Q_ASSERT(docs.contains(name));
        return docs[name].metadata;
    }

    QIcon icon(const QString& docsetName) {
        docsetEntry *entry = &docs[docsetName];
        QString bundleName = entry->info.bundleName;
        bundleName.replace(" ", "_");
        QString identifier = entry->info.bundleIdentifier;
        QIcon icon(entry->dir.absoluteFilePath("favicon.ico"));
        if(icon.availableSizes().isEmpty()) {
            icon = QIcon(entry->dir.absoluteFilePath("icon.png"));
        }
        if(icon.availableSizes().isEmpty()) {
#ifdef WIN32
            QDir icondir(QCoreApplication::applicationDirPath());
            icondir.cd("icons");
#else
            QDir icondir("/usr/share/pixmaps/zeal");
#endif
            icon = QIcon(icondir.filePath(bundleName+".png"));

            // Fallback to identifier and docset file name.
            if (icon.availableSizes().isEmpty()) {
                icon = QIcon(icondir.filePath(identifier+".png"));
            }
            if (icon.availableSizes().isEmpty()) {
                icon = QIcon(icondir.filePath(docsetName+".png"));
            }
        }
        return icon;
    }

    DocSetType type(const QString& name) const {
        Q_ASSERT(docs.contains(name));
        return docs[name].type;
    }

    QStringList names() const {
        return docs.keys();
    }

    void remove(const QString& name) {
        docs[name].db.close();
        docs.remove(name);
    }

    void clear() {
        for(auto key : docs.keys()) {
            remove(key);
        }
    }

    docsetEntry *getEntry(const QString &name);
    // Returns the list of links available in a given webpage.
    // Scans the list of related links for a given page. This lets you view the methods of a given object.
    QList<ZealSearchResult> getRelatedLinks(QString name, QString path);
    QString prepareQuery(const QString& rawQuery);
    void runQuery(const QString& query);
    void invalidateQueries();
    const QList<ZealSearchResult>& getQueryResults();
    QList<docsetEntry> docsets();

    QString docsetsDir();
    void initialiseDocsets();
    void setPrefixes(QHash<QString, QVariant> docsetPrefixes);
signals:
    void queryCompleted();

public slots:
    void addDocset(const QString& path);

private slots:
    void _runQuery(const QString& query, int queryNum);

private:
    ZealDocsetsRegistry();
    ZealDocsetsRegistry(const ZealDocsetsRegistry&); // hide copy constructor
    ZealDocsetsRegistry& operator=(const ZealDocsetsRegistry&); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident
    void addDocsetsFromFolder(QDir folder);

    static ZealDocsetsRegistry* m_Instance;
    QMap<QString, docsetEntry> docs;
    QList<ZealSearchResult> queryResults;
    QSettings settings;
    int lastQuery;
    void normalizeName(QString &itemName, QString &parentName, QString initialParent);
};

extern ZealDocsetsRegistry* docsets;

#endif // ZEALDOCSETSREGISTRY_H
