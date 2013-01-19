#include "zealdocsetsregistry.h"

ZealDocsetsRegistry* ZealDocsetsRegistry::m_Instance;

ZealDocsetsRegistry* docsets = ZealDocsetsRegistry::instance();

void ZealDocsetsRegistry::addDocset(const QString& path) {
    auto dir = QDir(path);
    auto db = QSqlDatabase::addDatabase("QSQLITE", dir.dirName());
    db.setDatabaseName(dir.filePath("index.sqlite"));
    db.open();
    dbs.insert(dir.dirName(), db);
    dirs.insert(dir.dirName(), dir);
}
