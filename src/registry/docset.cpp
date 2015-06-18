#include "docset.h"

#include "docsetinfo.h"
#include "searchquery.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

using namespace Zeal;

Docset::Docset(const QString &path) :
    m_path(path)
{
    QDir dir(m_path);
    if (!dir.exists())
        return;

    loadMetadata();

    // Attempt to find the icon in any supported format
    for (const QString &iconFile : dir.entryList({QStringLiteral("icon.*")}, QDir::Files)) {
        m_icon = QIcon(dir.absoluteFilePath(iconFile));
        if (!m_icon.availableSizes().isEmpty())
            break;
    }

    /// TODO: Report errors here and below
    if (!dir.cd(QStringLiteral("Contents")))
        return;

    DocsetInfo info;
    if (dir.exists(QStringLiteral("Info.plist")))
        info = DocsetInfo::fromPlist(dir.absoluteFilePath(QStringLiteral("Info.plist")));
    else if (dir.exists(QStringLiteral("info.plist")))
        info = DocsetInfo::fromPlist(dir.absoluteFilePath(QStringLiteral("info.plist")));
    else
        return;

    if (m_name.isEmpty()) {
        // Fallback if meta.json is absent
        if (!info.bundleName.isEmpty()) {
            m_name = m_title = info.bundleName;
            /// TODO: Remove when MainWindow::docsetName() will not use directory name
            m_name.replace(QLatin1Char(' '), QLatin1Char('_'));
        } else {
            m_name = QFileInfo(m_path).fileName().remove(QStringLiteral(".docset"));
        }
    }

    if (m_title.isEmpty()) {
        m_title = m_name;
        m_title.replace(QLatin1Char('_'), QLatin1Char(' '));
    }

    /// TODO: Verify if this is needed
    if (info.family == QLatin1String("cheatsheet"))
        m_name = m_name + QLatin1String("cheats");

    if (!dir.cd(QStringLiteral("Resources")) || !dir.exists(QStringLiteral("docSet.dsidx")))
        return;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
    db.setDatabaseName(dir.absoluteFilePath(QStringLiteral("docSet.dsidx")));

    if (!db.open()) {
        qWarning("SQL Error: %s", qPrintable(db.lastError().text()));
        return;
    }

    m_type = db.tables().contains(QStringLiteral("searchIndex")) ? Type::Dash : Type::ZDash;

    if (!dir.cd(QStringLiteral("Documents")))
        return;

    m_keyword = (info.bundleName.isEmpty() ? m_name : info.bundleName).toLower();

    // Try to find index path if metadata is missing one
    if (m_indexFilePath.isEmpty()) {
        if (!info.indexFilePath.isEmpty() && dir.exists(info.indexFilePath))
            m_indexFilePath = info.indexFilePath;
        else if (dir.exists(QStringLiteral("index.html")))
            m_indexFilePath = QStringLiteral("index.html");
        else
            qWarning("Cannot determine index file for docset %s", qPrintable(m_name));
    }

    countSymbols();
}

Docset::~Docset()
{
    QSqlDatabase::removeDatabase(m_name);
}

bool Docset::isValid() const
{
    return m_type != Type::Invalid;
}

QString Docset::name() const
{
    return m_name;
}

QString Docset::title() const
{
    return m_title;
}

QString Docset::keyword() const
{
    return m_keyword;
}

QString Docset::version() const
{
    return m_version;
}

QString Docset::revision() const
{
    return m_revision;
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
    return QDir(documentPath()).absoluteFilePath(m_indexFilePath);
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

QList<SearchResult> Docset::search(const QString &query) const
{
    QList<SearchResult> results;

    const SearchQuery searchQuery = SearchQuery::fromString(query);
    const QString sanitizedQuery = searchQuery.sanitizedQuery();

    if (searchQuery.hasKeywords() && !searchQuery.hasKeyword(m_keyword))
        return results;

    QString queryStr;

    bool withSubStrings = false;
    // %.%1% for long Django docset values like django.utils.http
    // %::%1% for long C++ docset values like std::set
    // %/%1% for long Go docset values like archive/tar
    QString subNames = QStringLiteral(" OR %1 LIKE '%.%2%' ESCAPE '\\'");
    subNames += QLatin1String(" OR %1 LIKE '%::%2%' ESCAPE '\\'");
    subNames += QLatin1String(" OR %1 LIKE '%/%2%' ESCAPE '\\'");
    while (results.size() < 100) {
        QString curQuery = sanitizedQuery;
        QString notQuery; // don't return the same result twice
        if (withSubStrings) {
            // if less than 100 found starting with query, search all substrings
            curQuery = QLatin1Char('%') + sanitizedQuery;
            // don't return 'starting with' results twice
            if (m_type == Docset::Type::Dash)
                notQuery = QString(" AND NOT (name LIKE '%1%' ESCAPE '\\' %2) ").arg(sanitizedQuery, subNames.arg("name", sanitizedQuery));
            else
                notQuery = QString(" AND NOT (ztokenname LIKE '%1%' ESCAPE '\\' %2) ").arg(sanitizedQuery, subNames.arg("ztokenname", sanitizedQuery));
        }
        if (m_type == Docset::Type::Dash) {
            queryStr = QString("SELECT name, type, path "
                               "    FROM searchIndex "
                               "WHERE (name LIKE '%1%' ESCAPE '\\' %3) %2 "
                               "LIMIT 100")
                    .arg(curQuery, notQuery, subNames.arg("name", curQuery));
        } else {
            queryStr = QString("SELECT ztokenname, ztypename, zpath, zanchor "
                               "    FROM ztoken "
                               "JOIN ztokenmetainformation "
                               "    ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                               "JOIN zfilepath "
                               "    ON ztokenmetainformation.zfile = zfilepath.z_pk "
                               "JOIN ztokentype "
                               "    ON ztoken.ztokentype = ztokentype.z_pk "
                               "WHERE (ztokenname LIKE '%1%' ESCAPE '\\' %3) %2 "
                               "LIMIT 100").arg(curQuery, notQuery,
                                                subNames.arg("ztokenname", curQuery));
        }

        QSqlQuery query(queryStr, database());
        while (query.next()) {
            const QString itemName = query.value(0).toString();
            QString path = query.value(2).toString();
            if (m_type == Docset::Type::ZDash) {
                const QString anchor = query.value(3).toString();
                if (!anchor.isEmpty())
                    path += QLatin1Char('#') + anchor;
            }

            /// TODO: Third should be type
            results.append(SearchResult{itemName, QString(),
                                        parseSymbolType(query.value(1).toString()),
                                        const_cast<Docset *>(this), path, sanitizedQuery});
        }

        if (withSubStrings)
            break;
        withSubStrings = true;  // try again searching for substrings
    }

    return results;
}

QList<SearchResult> Docset::relatedLinks(const QUrl &url) const
{
    QList<SearchResult> results;

    // Strip docset path and anchor from url
    const QString dir = documentPath();
    QString urlPath = url.path();
    int dirPosition = urlPath.indexOf(dir);
    QString path = url.path().mid(dirPosition + dir.size() + 1);

    // Get the url without the #anchor.
    QUrl cleanUrl(path);
    cleanUrl.setFragment(QString());

    // Prepare the query to look up all pages with the same url.
    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT name, type, path FROM searchIndex "
                                  "WHERE path LIKE \"%1%%\" AND path <> \"%1\"");
    } else if (m_type == Docset::Type::ZDash) {
        queryStr = QStringLiteral("SELECT ztoken.ztokenname, ztokentype.ztypename, zfilepath.zpath, ztokenmetainformation.zanchor "
                                  "FROM ztoken "
                                  "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk "
                                  "WHERE zfilepath.zpath = \"%1\" AND ztokenmetainformation.zanchor IS NOT NULL");
    }

    QSqlQuery query(queryStr.arg(cleanUrl.toString()), database());

    while (query.next()) {
        const QString sectionName = query.value(0).toString();
        QString sectionPath = query.value(2).toString();
        if (m_type == Docset::Type::ZDash) {
            sectionPath += QLatin1Char('#');
            sectionPath += query.value(3).toString();
        }

        results.append(SearchResult{sectionName, QString(),
                                    parseSymbolType(query.value(1).toString()),
                                    const_cast<Docset *>(this), sectionPath, QString()});
    }

    if (results.size() == 1)
        results.clear();

    return results;
}

QSqlDatabase Docset::database() const
{
    return QSqlDatabase::database(m_name, true);
}

void Docset::loadMetadata()
{
    const QDir dir(m_path);

    // Fallback if meta.json is absent
    if (!dir.exists(QStringLiteral("meta.json")))
        return;

    QScopedPointer<QFile> file(new QFile(dir.filePath(QStringLiteral("meta.json"))));
    if (!file->open(QIODevice::ReadOnly))
        return;

    QJsonParseError jsonError;
    const QJsonObject jsonObject = QJsonDocument::fromJson(file->readAll(), &jsonError).object();

    if (jsonError.error != QJsonParseError::NoError)
        return;

    m_name = jsonObject[QStringLiteral("name")].toString();
    m_title = jsonObject[QStringLiteral("title")].toString();
    m_version = jsonObject[QStringLiteral("version")].toString();
    m_revision = jsonObject[QStringLiteral("revision")].toString();

    if (jsonObject.contains(QStringLiteral("extra"))) {
        const QJsonObject extra = jsonObject[QStringLiteral("extra")].toObject();
        m_indexFilePath = extra[QStringLiteral("indexFilePath")].toString();
    }
}

void Docset::countSymbols()
{
    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT type, COUNT(*) FROM searchIndex GROUP BY type");
    } else if (m_type == Docset::Type::ZDash) {
        queryStr = QStringLiteral("SELECT ztypename, COUNT(*) FROM ztoken JOIN ztokentype"
                                  " ON ztoken.ztokentype = ztokentype.z_pk GROUP BY ztypename");
    }

    QSqlQuery query(queryStr, database());
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
void Docset::loadSymbols(const QString &symbolType) const
{
    for (const QString &symbol : m_symbolStrings.values(symbolType))
        loadSymbols(symbolType, symbol);
}

void Docset::loadSymbols(const QString &symbolType, const QString &symbolString) const
{
    QSqlDatabase db = database();
    if (!db.isOpen())
        return;

    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT name, path FROM searchIndex WHERE type='%1' ORDER BY name ASC");
    } else {
        queryStr = QStringLiteral("SELECT ztokenname AS name, "
                                  "CASE WHEN (zanchor IS NULL) THEN zpath "
                                  "ELSE (zpath || '#' || zanchor) "
                                  "END AS path FROM ztoken "
                                  "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk WHERE ztypename='%1' "
                                  "ORDER BY ztokenname ASC");
    }

    QSqlQuery query(queryStr.arg(symbolString), database());
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
        // Attribute
        {QStringLiteral("Package Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Private Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Protected Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Public Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Static Package Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Static Private Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Static Protected Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("Static Public Attributes"), QStringLiteral("Attribute")},
        {QStringLiteral("XML Attributes"), QStringLiteral("Attribute")},
        // Binding
        {QStringLiteral("binding"), QStringLiteral("Binding")},
        // Category
        {QStringLiteral("cat"), QStringLiteral("Category")},
        {QStringLiteral("Groups"), QStringLiteral("Category")},
        {QStringLiteral("Pages"), QStringLiteral("Category")},
        // Class
        {QStringLiteral("cl"), QStringLiteral("Class")},
        {QStringLiteral("specialization"), QStringLiteral("Class")},
        {QStringLiteral("tmplt"), QStringLiteral("Class")},
        // Constant
        {QStringLiteral("data"), QStringLiteral("Constant")},
        {QStringLiteral("econst"), QStringLiteral("Constant")},
        {QStringLiteral("enumelt"), QStringLiteral("Constant")},
        {QStringLiteral("clconst"), QStringLiteral("Constant")},
        {QStringLiteral("structdata"), QStringLiteral("Constant")},
        {QStringLiteral("Notifications"), QStringLiteral("Constant")},
        // Constructor
        {QStringLiteral("Public Constructors"), QStringLiteral("Constructor")},
        // Enumeration
        {QStringLiteral("enum"), QStringLiteral("Enumeration")},
        {QStringLiteral("Enum"), QStringLiteral("Enumeration")},
        {QStringLiteral("Enumerations"), QStringLiteral("Enumeration")},
        // Event
        {QStringLiteral("event"), QStringLiteral("Event")},
        {QStringLiteral("Public Events"), QStringLiteral("Event")},
        {QStringLiteral("Inherited Events"), QStringLiteral("Event")},
        {QStringLiteral("Private Events"), QStringLiteral("Event")},
        // Field
        {QStringLiteral("Data Fields"), QStringLiteral("Field")},
        // Function
        {QStringLiteral("dcop"), QStringLiteral("Function")},
        {QStringLiteral("func"), QStringLiteral("Function")},
        {QStringLiteral("ffunc"), QStringLiteral("Function")},
        {QStringLiteral("signal"), QStringLiteral("Function")},
        {QStringLiteral("slot"), QStringLiteral("Function")},
        {QStringLiteral("grammar"), QStringLiteral("Function")},
        {QStringLiteral("Function Prototypes"), QStringLiteral("Function")},
        {QStringLiteral("Functions/Subroutines"), QStringLiteral("Function")},
        {QStringLiteral("Members"), QStringLiteral("Function")},
        {QStringLiteral("Package Functions"), QStringLiteral("Function")},
        {QStringLiteral("Private Member Functions"), QStringLiteral("Function")},
        {QStringLiteral("Private Slots"), QStringLiteral("Function")},
        {QStringLiteral("Protected Member Functions"), QStringLiteral("Function")},
        {QStringLiteral("Protected Slots"), QStringLiteral("Function")},
        {QStringLiteral("Public Member Functions"), QStringLiteral("Function")},
        {QStringLiteral("Public Slots"), QStringLiteral("Function")},
        {QStringLiteral("Signals"), QStringLiteral("Function")},
        {QStringLiteral("Static Package Functions"), QStringLiteral("Function")},
        {QStringLiteral("Static Private Member Functions"), QStringLiteral("Function")},
        {QStringLiteral("Static Protected Member Functions"), QStringLiteral("Function")},
        {QStringLiteral("Static Public Member Functions"), QStringLiteral("Function")},
        // Guide
        {QStringLiteral("doc"), QStringLiteral("Guide")},
        // Namespace
        {QStringLiteral("ns"), QStringLiteral("Namespace")},
        // Macro
        {QStringLiteral("macro"), QStringLiteral("Macro")},
        // Method
        {QStringLiteral("clm"), QStringLiteral("Method")},
        {QStringLiteral("intfctr"), QStringLiteral("Method")},
        {QStringLiteral("intfcm"), QStringLiteral("Method")},
        {QStringLiteral("intfm"), QStringLiteral("Method")},
        {QStringLiteral("instctr"), QStringLiteral("Method")},
        {QStringLiteral("instm"), QStringLiteral("Method")},
        {QStringLiteral("Class Methods"), QStringLiteral("Method")},
        {QStringLiteral("Inherited Methods"), QStringLiteral("Method")},
        {QStringLiteral("Instance Methods"), QStringLiteral("Method")},
        {QStringLiteral("Private Methods"), QStringLiteral("Method")},
        {QStringLiteral("Protected Methods"), QStringLiteral("Method")},
        {QStringLiteral("Public Methods"), QStringLiteral("Method")},
        // Property
        {QStringLiteral("intfp"), QStringLiteral("Property")},
        {QStringLiteral("instp"), QStringLiteral("Property")},
        {QStringLiteral("Inherited Properties"), QStringLiteral("Property")},
        {QStringLiteral("Private Properties"), QStringLiteral("Property")},
        {QStringLiteral("Protected Properties"), QStringLiteral("Property")},
        {QStringLiteral("Public Properties"), QStringLiteral("Property")},
        // Protocol
        {QStringLiteral("intf"), QStringLiteral("Protocol")},
        // Structure
        {QStringLiteral("struct"), QStringLiteral("Structure")},
        {QStringLiteral("Data Structures"), QStringLiteral("Structure")},
        {QStringLiteral("Struct"), QStringLiteral("Structure")},
        // Type
        {QStringLiteral("tag"), QStringLiteral("Type")},
        {QStringLiteral("tdef"), QStringLiteral("Type")},
        {QStringLiteral("Data Types"), QStringLiteral("Type")},
        {QStringLiteral("Package Types"), QStringLiteral("Type")},
        {QStringLiteral("Private Types"), QStringLiteral("Type")},
        {QStringLiteral("Protected Types"), QStringLiteral("Type")},
        {QStringLiteral("Public Types"), QStringLiteral("Type")},
        {QStringLiteral("Typedefs"), QStringLiteral("Type")},
        // Variable
        {QStringLiteral("var"), QStringLiteral("Variable")}
    };

    return aliases.value(str, str);
}
