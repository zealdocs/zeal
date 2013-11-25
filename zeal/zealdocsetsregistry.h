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

#include "zealsearchresult.h"

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
        return dbs.count();
    }

    QSqlDatabase& db(const QString& name) {
        return dbs[name];
    }

    const QDir& dir(const QString& name) {
        return dirs[name];
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
        return types[name];
    }

    QStringList names() const {
        return dbs.keys();
    }

    void remove(const QString& name) {
        dbs[name].close();
        dbs.remove(name);
        dirs.remove(name);
        types.remove(name);
    }

    void clear() {
        for(auto key : dbs.keys()) {
            remove(key);
        }
    }

    QString prepareQuery(const QString& rawQuery);
    void runQuery(const QString& query);
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
    ZealDocsetsRegistry();
    ZealDocsetsRegistry(const ZealDocsetsRegistry&); // hide copy constructor
    ZealDocsetsRegistry& operator=(const ZealDocsetsRegistry&); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident

    static ZealDocsetsRegistry* m_Instance;
    // FIXME: DocSet class could be better instead of 3 maps
    QMap<QString, QSqlDatabase> dbs;
    QMap<QString, QDir> dirs;
    QMap<QString, DocSetType> types;
    QList<ZealSearchResult> queryResults;
    QSettings settings;
    int lastQuery;
};

extern ZealDocsetsRegistry* docsets;

#endif // ZEALDOCSETSREGISTRY_H
