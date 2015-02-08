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

    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
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
    db.close();
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

QMap<Docset::SymbolType, int> Docset::symbolCounts() const
{
    return m_symbolCounts;
}

int Docset::symbolCount(Docset::SymbolType type) const
{
    return m_symbolCounts.value(type);
}

int Docset::symbolCount(const QString &typeStr) const
{
    return m_symbolCounts.value(strToSymbolType(typeStr));
}

const QMap<QString, QString> &Docset::symbols(Docset::SymbolType type) const
{
    if (!m_symbols.contains(type))
        loadSymbols(type);
    return m_symbols[type];
}

/// TODO: Remove after refactoring in ListModel
QString Docset::symbolTypeToStr(SymbolType symbolType)
{
    QMetaEnum types = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("SymbolType"));
    return types.valueToKey(static_cast<int>(symbolType));
}

/// TODO: Make private
Docset::SymbolType Docset::strToSymbolType(const QString &str)
{
    const static QHash<QString, SymbolType> typeStrings = {
        {QStringLiteral("cl"), SymbolType::Class},
        {QStringLiteral("clconst"), SymbolType::Constant},
        {QStringLiteral("enum"), SymbolType::Enumeration},
        {QStringLiteral("func"), SymbolType::Function},
        {QStringLiteral("clm"), SymbolType::Method},
        {QStringLiteral("struct"), SymbolType::Structure},
        {QStringLiteral("tdef"), SymbolType::Type}
    };

    QString typeStr = str.toLower();
    if (typeStrings.contains(typeStr))
        return typeStrings.value(typeStr);

    QMetaEnum types = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("SymbolType"));

    bool ok;
    SymbolType type = static_cast<SymbolType>(types.keyToValue(qPrintable(str), &ok));
    if (ok)
        return type;

    typeStr[0] = typeStr[0].toUpper();
    type = static_cast<SymbolType>(types.keyToValue(qPrintable(typeStr), &ok));
    if (ok)
        return type;

    qWarning("Unknown symbol: %s", qPrintable(str));
    return SymbolType::Unknown;
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
        const SymbolType symbolType = strToSymbolType(symbolTypeStr);
        if (symbolType == SymbolType::Unknown)
            continue;

        m_symbolStrings.insertMulti(symbolType, symbolTypeStr);
        m_symbolCounts[symbolType] += query.value(1).toInt();
    }
}

/// TODO: Fetch and cache only portions of symbols
void Docset::loadSymbols(SymbolType symbolType) const
{
    for (const QString &symbol : m_symbolStrings.values(symbolType))
        loadSymbols(symbolType, symbol);
}

void Docset::loadSymbols(SymbolType symbolType, const QString &symbolString) const
{
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
