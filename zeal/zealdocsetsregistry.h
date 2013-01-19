#ifndef ZEALDOCSETSREGISTRY_H
#define ZEALDOCSETSREGISTRY_H

#include <QMutex>
#include <QtSql/QSqlDatabase>
#include <QDir>
#include <QMap>
#include <iostream>
using namespace std;

class ZealDocsetsRegistry
{
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

    void addDocset(const QString& path);

    int count() const {
        return dbs.count();
    }

    QSqlDatabase& db(const QString& name) {
        return dbs[name];
    }

    const QDir& dir(const QString& name) {
        return dirs[name];
    }

    QStringList names() const {
        return dbs.keys();
    }

private:
    ZealDocsetsRegistry() {
    }

    ZealDocsetsRegistry(const ZealDocsetsRegistry&); // hide copy constructor
    ZealDocsetsRegistry& operator=(const ZealDocsetsRegistry&); // hide assign op
                                 // we leave just the declarations, so the compiler will warn us
                                 // if we try to use those two functions by accident

    static ZealDocsetsRegistry* m_Instance;
    QMap<QString, QSqlDatabase> dbs;
    QMap<QString, QDir> dirs;
};

extern ZealDocsetsRegistry* docsets;

#endif // ZEALDOCSETSREGISTRY_H
