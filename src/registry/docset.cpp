#include "docset.h"

#include <QDir>
#include <QSqlQuery>
#include <QVariant>

using namespace Zeal;

Docset::Docset()
{
}

Docset::Docset(const QString &path) :
    m_path(path)
{
    QDir dir(m_path);
    if (!dir.exists())
        return;

    /// TODO: Use metadata
    m_name = dir.dirName().replace(QStringLiteral(".docset"), QString());

    /// TODO: Report errors here and below
    if (!dir.cd(QStringLiteral("Contents")))
        return;

    if (dir.exists(QStringLiteral("Info.plist")))
        info = DocsetInfo::fromPlist(dir.absoluteFilePath(QStringLiteral("Info.plist")));
    else if (dir.exists(QStringLiteral("info.plist")))
        info = DocsetInfo::fromPlist(dir.absoluteFilePath(QStringLiteral("info.plist")));
    else
        return;

    // Read metadata
    metadata = DocsetMetadata::fromFile(path + QStringLiteral("/meta.json"));

    if (info.family == QStringLiteral("cheatsheet"))
        m_name = QString(QStringLiteral("%1_cheats")).arg(m_name);

    if (!dir.cd(QStringLiteral("Resources")))
        return;

    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
    db.setDatabaseName(dir.absoluteFilePath(QStringLiteral("docSet.dsidx")));

    if (!db.open())
        return;

    QSqlQuery q = db.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'"));

    type = Docset::Type::ZDash;
    while (q.next()) {
        if (q.value(0).toString() == QStringLiteral("searchIndex")) {
            type = Docset::Type::Dash;
            break;
        }
    }

    if (!dir.cd(QStringLiteral("Documents")))
        return;

    prefix = info.bundleName.isEmpty() ? m_name : info.bundleName;

    findIcon();

    m_isValid = true;
}

Docset::~Docset()
{
}

bool Docset::isValid() const
{
    return m_isValid;
}

QString Docset::name() const
{
    return m_name;
}

QString Docset::path() const
{
    return m_path;
}

QString Docset::documentPath() const
{
    return QDir(m_path).absoluteFilePath(QStringLiteral("Contents/Resources/Documents"));
}

QIcon Docset::icon() const
{
    return m_icon;
}

void Docset::findIcon()
{
    const QDir dir(m_path);
    for (const QString &iconFile : dir.entryList({QStringLiteral("icon.*")}, QDir::Files)) {
        m_icon = QIcon(dir.absoluteFilePath(iconFile));
        if (!m_icon.availableSizes().isEmpty())
            return;
    }

    QString bundleName = info.bundleName;
    bundleName.replace(QLatin1String(" "), QLatin1String("_"));
    m_icon = QIcon(QString(QStringLiteral("icons:%1.png")).arg(bundleName));
    if (!m_icon.availableSizes().isEmpty())
        return;

    // Fallback to identifier and docset file name.
    m_icon = QIcon(QString(QStringLiteral("icons:%1.png")).arg(info.bundleIdentifier));
    if (!m_icon.availableSizes().isEmpty())
        return;

    m_icon = QIcon(QString(QStringLiteral("icons:%1.png")).arg(m_name));
    if (!m_icon.availableSizes().isEmpty())
        return;
}

