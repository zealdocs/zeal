/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "docset.h"
#include "docsetsearchstrategy.h"
#include "searchresult.h"

#include "searchquery.h"
#include "util/plist.h"

#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

using namespace Zeal;

namespace {
const char IndexNamePrefix[] = "__zi_name"; // zi - Zeal index
const char IndexNameVersion[] = "0001"; // Current index version

namespace InfoPlist {
const char CFBundleName[] = "CFBundleName";
const char CFBundleIdentifier[] = "CFBundleIdentifier";
const char DashDocSetFamily[] = "DashDocSetFamily";
const char DashDocSetKeyword[] = "DashDocSetKeyword";
const char DashDocSetPluginKeyword[] = "DashDocSetPluginKeyword";
const char DashIndexFilePath[] = "dashIndexFilePath";
const char DocSetPlatformFamily[] = "DocSetPlatformFamily";
const char IsDashDocset[] = "isDashDocset";
const char IsJavaScriptEnabled[] = "isJavaScriptEnabled";
}
}

namespace Zeal {

class DashSearchStrategy : public DocsetSearchStrategy
{
public:
    explicit DashSearchStrategy(Docset *docset);
    QList<SearchResult> search(const QString &query) const override;

private:
    Docset *m_docset;
};

}

DashSearchStrategy::DashSearchStrategy(Docset *docset)
    : m_docset(docset)
{
}

QList<SearchResult> DashSearchStrategy::search(const QString &rawQuery) const
{
    QList<SearchResult> results;
    int resultCount = 0;

    const SearchQuery searchQuery = SearchQuery::fromString(rawQuery);
    const QString sanitizedQuery = searchQuery.sanitizedQuery();

    if (searchQuery.hasKeywords() && !searchQuery.hasKeywords(m_docset->keywords()))
        return QList<SearchResult>();

    QString queryStr;

    // Search all substrings
    QString curQuery = QLatin1Char('%') + sanitizedQuery;
    if (m_docset->type() == Docset::Type::Dash) {
        queryStr = QString("SELECT name, type, path "
                           "    FROM searchIndex "
                           "WHERE (name LIKE :query ESCAPE '\\') ");
    } else {
        queryStr = QString("SELECT ztokenname, ztypename, zpath, zanchor "
                           "    FROM ztoken "
                           "JOIN ztokenmetainformation "
                           "    ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                           "JOIN zfilepath "
                           "    ON ztokenmetainformation.zfile = zfilepath.z_pk "
                           "JOIN ztokentype "
                           "    ON ztoken.ztokentype = ztokentype.z_pk "
                           "WHERE (ztokenname LIKE :query ESCAPE '\\') ");
    }

    QSqlQuery query(m_docset->database());
    query.prepare(queryStr);
    query.bindValue(":query", QString("%1%").arg(curQuery));
    query.exec();

    while (query.next() && resultCount < Docset::MaxDocsetResultsCount) {
        const QString itemName = query.value(0).toString();
        QString path = query.value(2).toString();
        if (m_docset->type() == Docset::Type::ZDash) {
            const QString anchor = query.value(3).toString();
            if (!anchor.isEmpty())
                path += QLatin1Char('#') + anchor;
        }

        int score = Docset::scoreSubstringResult(searchQuery, itemName);
        /// TODO: Third should be type
        SearchResult newResult(SearchResult{itemName, QString(),
                               m_docset->parseSymbolType(query.value(1).toString()),
                               const_cast<Docset *>(m_docset),
                               path, sanitizedQuery, score});

        results << newResult;
        resultCount++;
    }

    return results;
}

Docset::Docset(const QString &path) :
    m_path(path),
    m_searchStrategy(new DashSearchStrategy(this))
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

    /// TODO: 'info.plist' is invalid according to Apple, and must alsways be 'Info.plist'
    /// https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPRuntimeConfig/
    /// Articles/ConfigFiles.html
    Util::Plist plist;
    if (dir.exists(QStringLiteral("Info.plist")))
        plist.read(dir.absoluteFilePath(QStringLiteral("Info.plist")));
    else if (dir.exists(QStringLiteral("info.plist")))
        plist.read(dir.absoluteFilePath(QStringLiteral("info.plist")));
    else
        return;

    if (plist.hasError())
        return;

    if (m_name.isEmpty()) {
        // Fallback if meta.json is absent
        if (!plist.contains(InfoPlist::CFBundleName)) {
            m_name = m_title = plist[InfoPlist::CFBundleName].toString();
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
    if (plist.contains(InfoPlist::DashDocSetFamily)
            && plist[InfoPlist::DashDocSetFamily].toString() == QLatin1String("cheatsheet")) {
        m_name = m_name + QLatin1String("cheats");
    }

    if (!dir.cd(QStringLiteral("Resources")) || !dir.exists(QStringLiteral("docSet.dsidx")))
        return;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
    db.setDatabaseName(dir.absoluteFilePath(QStringLiteral("docSet.dsidx")));

    if (!db.open()) {
        qWarning("SQL Error: %s", qPrintable(db.lastError().text()));
        return;
    }

    m_type = db.tables().contains(QStringLiteral("searchIndex")) ? Type::Dash : Type::ZDash;

    createIndex();

    if (!dir.cd(QStringLiteral("Documents")))
        return;

    //
    // Setyp keywords
    if (plist.contains(InfoPlist::DocSetPlatformFamily))
        m_keywords << plist[InfoPlist::DocSetPlatformFamily].toString();

    if (plist.contains(InfoPlist::DashDocSetPluginKeyword))
        m_keywords << plist[InfoPlist::DashDocSetPluginKeyword].toString();

    if (plist.contains(InfoPlist::DashDocSetKeyword))
        m_keywords << plist[InfoPlist::DashDocSetKeyword].toString();

    if (plist.contains(InfoPlist::DashDocSetFamily)) {
        const QString kw = plist[InfoPlist::DashDocSetFamily].toString();
        if (kw != QStringLiteral("dashtoc"))
            m_keywords << kw;
    }

    /// TODO: Use 'unknown' instead of CFBundleName? (See #383)
    m_keywords << plist.value(InfoPlist::CFBundleName, m_name).toString().toLower();

    // Try to find index path if metadata is missing one
    if (m_indexFilePath.isEmpty()) {
        if (plist.contains(InfoPlist::DashIndexFilePath))
            m_indexFilePath = plist[InfoPlist::DashIndexFilePath].toString();
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

QStringList Docset::keywords() const
{
    return m_keywords;
}

QString Docset::version() const
{
    return m_version;
}

QString Docset::revision() const
{
    return m_revision;
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

Docset::Type Docset::type() const
{
    return m_type;
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

bool Docset::endsWithSeparator(QString result, int pos)
{
    QString beforeMatch = result.mid(0, pos);
    return beforeMatch.endsWith(".") || beforeMatch.endsWith("::") || beforeMatch.endsWith("/");
}

int Docset::separators(QString result, int pos)
{
    QRegularExpression separator("\\.|::|/");
    return result.mid(0, pos).count(separator);
}

int Docset::scoreSubstringResult(const SearchQuery &query, const QString result)
{
    int score = TotalBuckets - 1;

    int index = result.indexOf(query.query());
    if (index == 0 || Docset::endsWithSeparator(result, index)) {
        score -= result.size() - query.query().size() - index + Docset::separators(result, index);
    } else {
        score -= result.size() - query.query().size() + 2;
    }

    return std::max(1, score);
}

QList<SearchResult> Docset::search(const QString &rawQuery) const
{
    return m_searchStrategy->search(rawQuery);
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
                                    const_cast<Docset *>(this), sectionPath, QString(), 0});
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

    std::unique_ptr<QFile> file(new QFile(dir.filePath(QStringLiteral("meta.json"))));
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
        m_symbolsTotal += query.value(1).toInt();
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

void Docset::createIndex()
{
    static const QString indexListQuery = QStringLiteral("PRAGMA INDEX_LIST('%1')");
    static const QString indexDropQuery = QStringLiteral("DROP INDEX '%1'");
    static const QString indexCreateQuery = QStringLiteral("CREATE INDEX IF NOT EXISTS %1%2"
                                                           " ON %3 (name COLLATE NOCASE)");

    QSqlQuery query(database());

    const QString tableName = m_type == Type::Dash ? QStringLiteral("searchIndex")
                                                   : QStringLiteral("ztoken");

    query.exec(indexListQuery.arg(tableName));

    QStringList oldIndexes;

    while (query.next()) {
        const QString indexName = query.value(1).toString();
        if (!indexName.startsWith(IndexNamePrefix))
            continue;

        if (indexName.endsWith(IndexNameVersion))
            return;

        oldIndexes << indexName;
    }

    // Drop old indexes
    for (const QString oldIndexName : oldIndexes)
        query.exec(indexDropQuery.arg(oldIndexName));

    query.exec(indexCreateQuery.arg(IndexNamePrefix, IndexNameVersion, tableName));
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
