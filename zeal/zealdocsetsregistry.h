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
#include "zealdocsetmetadata.h"

typedef enum {ZEAL, DASH, ZDASH} DocSetType;

class ZealDocsetsRegistry : public QObject
{
    Q_OBJECT
public:

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
        return docs[name].db;
    }

    const QDir& dir(const QString& name) {
        return docs[name].dir;
    }

    const ZealDocsetMetadata& meta(const QString& name){
        return docs[name].metadata;
    }

    QIcon icon(const QString& name) {
        QIcon icon(dir(name).absoluteFilePath("favicon.ico"));
        if(icon.availableSizes().isEmpty()) {
            icon = QIcon(dir(name).absoluteFilePath("icon.png"));
        }
        if(icon.availableSizes().isEmpty()) {
#ifdef WIN32
            QDir icondir(QCoreApplication::applicationDirPath());
            icondir.cd("icons");
#else
            QDir icondir("/usr/share/pixmaps/zeal");
#endif
            icon = QIcon(icondir.filePath(name+".png"));
        }
        return icon;
    }

    DocSetType type(const QString& name) const {
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

    QString prepareQuery(const QString& rawQuery);
    void runQuery(const QString& query);
    void invalidateQueries();
    const QList<ZealSearchResult>& getQueryResults();

    QString docsetsDir();
    void initialiseDocsets();

signals:
    void queryCompleted();

public slots:
    void addDocset(const QString& path);

private slots:
    void _runQuery(const QString& query, int queryNum);

private:
    typedef struct {
        QSqlDatabase db;
        QDir dir;
        DocSetType type;
        ZealDocsetMetadata metadata;
    } docsetEntry;

    ZealDocsetsRegistry();
    ZealDocsetsRegistry(const ZealDocsetsRegistry&); // hide copy constructor
    ZealDocsetsRegistry& operator=(const ZealDocsetsRegistry&); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident

    static ZealDocsetsRegistry* m_Instance;
    QMap<QString, docsetEntry> docs;
    QList<ZealSearchResult> queryResults;
    QSettings settings;
    int lastQuery;
};

extern ZealDocsetsRegistry* docsets;

#endif // ZEALDOCSETSREGISTRY_H
