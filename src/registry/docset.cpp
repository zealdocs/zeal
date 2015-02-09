#include "docset.h"

#include <QDir>
#include <QMetaEnum>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

using namespace Zeal;

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

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
    db.setDatabaseName(dir.absoluteFilePath(QStringLiteral("docSet.dsidx")));

    if (!db.open())
        return;

    QSqlQuery query = db.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'"));

    if (query.lastError().type() != QSqlError::NoError) {
        qWarning("SQL Error: %s", qPrintable(query.lastError().text()));
        return;
    }

    m_type = Docset::Type::ZDash;
    while (query.next()) {
        if (query.value(0).toString() == QStringLiteral("searchIndex")) {
            m_type = Docset::Type::Dash;
            break;
        }
    }

    if (!dir.cd(QStringLiteral("Documents")))
        return;

    prefix = info.bundleName.isEmpty() ? m_name : info.bundleName;

    findIcon();
    countSymbols();

    m_isValid = true;
}

Docset::~Docset()
{
    QSqlDatabase::removeDatabase(m_name);
}

bool Docset::isValid() const
{
    return m_isValid;
}

QString Docset::name() const
{
    return m_name;
}

Docset::Type Docset::type() const
{
    return m_type;
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

QString Docset::indexFilePath() const
{
    /// TODO: Check if file exists
    const QDir dir(documentPath());
    return dir.absoluteFilePath(info.indexPath.isEmpty() ? QStringLiteral("index.html") : info.indexPath);
}

QMap<QString, int> Docset::symbolCounts() const
{
    return m_symbolCounts;
}

int Docset::symbolCount(const QString &symbolType) const
{
    return m_symbolCounts.value(symbolType);
}

const QMap<QString, QString> &Docset::symbols(const QString &symbolType) const
{
    if (!m_symbols.contains(symbolType))
        loadSymbols(symbolType);
    return m_symbols[symbolType];
}

QSqlDatabase Docset::database() const
{
    return QSqlDatabase::database(m_name, true);
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
    m_icon = QIcon(QString(QStringLiteral("docsetIcon:%1.png")).arg(bundleName));
    if (!m_icon.availableSizes().isEmpty())
        return;

    // Fallback to identifier and docset file name.
    m_icon = QIcon(QString(QStringLiteral("docsetIcon:%1.png")).arg(info.bundleIdentifier));
    if (!m_icon.availableSizes().isEmpty())
        return;

    m_icon = QIcon(QString(QStringLiteral("docsetIcon:%1.png")).arg(m_name));
    if (!m_icon.availableSizes().isEmpty())
        return;
}

void Docset::countSymbols()
{
    QSqlDatabase db = database();
    if (!db.isOpen())
        return;

    QSqlQuery query;
    if (m_type == Docset::Type::Dash) {
        query = db.exec(QStringLiteral("SELECT type, COUNT(*) FROM searchIndex GROUP BY type"));
    } else if (m_type == Docset::Type::ZDash) {
        query = db.exec(QStringLiteral("SELECT ztypename, COUNT(*) FROM ztoken JOIN ztokentype"
                                       " ON ztoken.ztokentype = ztokentype.z_pk GROUP BY ztypename"));
    }

    if (query.lastError().type() != QSqlError::NoError) {
        qWarning("SQL Error: %s", qPrintable(query.lastError().text()));
        return;
    }

    while (query.next()) {
        const QString symbolTypeStr = query.value(0).toString();
        const QString symbolType = parseSymbolType(symbolTypeStr);
        m_symbolStrings.insertMulti(symbolType, symbolTypeStr);
        m_symbolCounts[symbolType] += query.value(1).toInt();
    }
}

/// TODO: Fetch and cache only portions of symbols
void Docset::loadSymbols(QString symbolType) const
{
    for (const QString &symbol : m_symbolStrings.values(symbolType))
        loadSymbols(symbolType, symbol);
}

void Docset::loadSymbols(QString symbolType, const QString &symbolString) const
{
    QSqlDatabase db = database();
    if (!db.isOpen())
        return;

    QString queryStr;
    switch (m_type) {
    case Docset::Type::Dash:
        queryStr = QString("SELECT name, path FROM searchIndex WHERE type='%1' ORDER BY name ASC")
                .arg(symbolString);
        break;
    case Docset::Type::ZDash:
        queryStr = QString("SELECT ztokenname AS name, "
                           "CASE WHEN (zanchor IS NULL) THEN zpath "
                           "ELSE (zpath || '#' || zanchor) "
                           "END AS path FROM ztoken "
                           "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                           "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                           "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk WHERE ztypename='%1' "
                           "ORDER BY ztokenname ASC").arg(symbolString);
        break;
    }


    QSqlQuery query = db.exec(queryStr);

    if (query.lastError().type() != QSqlError::NoError) {
        qWarning("SQL Error: %s", qPrintable(query.lastError().text()));
        return;
    }

    QMap<QString, QString> &symbols = m_symbols[symbolType];
    while (query.next())
        symbols.insertMulti(query.value(0).toString(), QDir(documentPath()).absoluteFilePath(query.value(1).toString()));
}

QString Docset::parseSymbolType(const QString &str)
{
    /// Dash symbol aliases
    const static QHash<QString, QString> aliases = {
        // Class
        {QStringLiteral("cl"), QStringLiteral("Class")},
        {QStringLiteral("specialization"), QStringLiteral("Class")},
        // Constant
        {QStringLiteral("clconst"), QStringLiteral("Constant")},
        // Enumeration
        {QStringLiteral("enum"), QStringLiteral("Enumeration")},
        {QStringLiteral("Enum"), QStringLiteral("Enumeration")},
        // Function
        {QStringLiteral("func"), QStringLiteral("Function")},
        // Macro
        {QStringLiteral("macro"), QStringLiteral("Macro")},
        // Method
        {QStringLiteral("clm"), QStringLiteral("Method")},
        // Structure
        {QStringLiteral("struct"), QStringLiteral("Structure")},
        {QStringLiteral("Struct"), QStringLiteral("Structure")},
        // Type
        {QStringLiteral("tdef"), QStringLiteral("Type")}
    };

    return aliases.value(str, str);
}
