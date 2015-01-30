#include "docset.h"

#include <QDir>
#include <QSqlQuery>
#include <QVariant>

using namespace Zeal;

Docset::Docset()
{
}

Docset::Docset(const QString &path)
{
    QDir dir(path);
    if (!dir.exists())
        return;

    /// TODO: Use metadata
    name = dir.dirName().replace(QStringLiteral(".docset"), QString());

    /// TODO: Report errors here and below
    if (!dir.cd("Contents"))
        return;

    info = DocsetInfo::fromPlist(dir.absoluteFilePath("Info.plist"));

    // Read metadata
    metadata = DocsetMetadata::fromFile(path + QStringLiteral("/meta.json"));

    if (info.family == QStringLiteral("cheatsheet"))
        name = QString("%1_cheats").arg(name);
    name = name;

    if (!dir.cd("Resources"))
        return;

    db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(dir.absoluteFilePath("docSet.dsidx"));

    if (!db.open())
        return;

    QSqlQuery q = db.exec("select name from sqlite_master where type='table'");

    type = Docset::Type::ZDash;
    while (q.next()) {
        if (q.value(0).toString() == QStringLiteral("searchIndex")) {
            type = Docset::Type::Dash;
            break;
        }
    }

    if (!dir.cd("Documents"))
        return;

    prefix = info.bundleName.isEmpty() ? name : info.bundleName;
    documentPath = dir.absolutePath();

    m_isValid = true;
}

Docset::~Docset()
{
}

bool Docset::isValid() const
{
    return m_isValid;
}

QIcon Docset::icon() const
{
    const QDir dir(documentPath);

    QIcon icon(dir.absoluteFilePath("favicon.ico"));

    if (icon.availableSizes().isEmpty())
        icon = QIcon(dir.absoluteFilePath("icon.png"));

    if (icon.availableSizes().isEmpty()) {
        QString bundleName = info.bundleName;
        bundleName.replace(" ", "_");

        icon = QIcon(QString("icons:%1.png").arg(bundleName));

        // Fallback to identifier and docset file name.
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(info.bundleIdentifier));
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(name));
    }
    return icon;
}

