/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "docset.h"

#include "cancellationtoken.h"
#include "searchresult.h"

#include <util/plist.h>
#include <util/sqlitedatabase.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QVariant>

#include <sqlite3.h>

using namespace Zeal::Registry;

namespace {
const char IndexNamePrefix[] = "__zi_name"; // zi - Zeal index
const char IndexNameVersion[] = "0001"; // Current index version

namespace InfoPlist {
const char CFBundleName[] = "CFBundleName";
//const char CFBundleIdentifier[] = "CFBundleIdentifier";
const char DashDocSetFamily[] = "DashDocSetFamily";
const char DashDocSetKeyword[] = "DashDocSetKeyword";
const char DashDocSetPluginKeyword[] = "DashDocSetPluginKeyword";
const char DashIndexFilePath[] = "dashIndexFilePath";
const char DocSetPlatformFamily[] = "DocSetPlatformFamily";
//const char IsDashDocset[] = "isDashDocset";
//const char IsJavaScriptEnabled[] = "isJavaScriptEnabled";
}
}

static void sqliteScoreFunction(sqlite3_context *context, int argc, sqlite3_value **argv);

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

    // TODO: Report errors here and below
    if (!dir.cd(QStringLiteral("Contents")))
        return;

    // TODO: 'info.plist' is invalid according to Apple, and must alsways be 'Info.plist'
    // https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPRuntimeConfig
    // /Articles/ConfigFiles.html
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
        if (plist.contains(InfoPlist::CFBundleName)) {
            m_name = m_title = plist[InfoPlist::CFBundleName].toString();
            // TODO: Remove when MainWindow::docsetName() will not use directory name
            m_name.replace(QLatin1Char(' '), QLatin1Char('_'));
        } else {
            m_name = QFileInfo(m_path).fileName().remove(QStringLiteral(".docset"));
        }
    }

    if (m_title.isEmpty()) {
        m_title = m_name;
        m_title.replace(QLatin1Char('_'), QLatin1Char(' '));
    }

    // TODO: Verify if this is needed
    if (plist.contains(InfoPlist::DashDocSetFamily)
            && plist[InfoPlist::DashDocSetFamily].toString() == QLatin1String("cheatsheet")) {
        m_name = m_name + QLatin1String("cheats");
    }

    if (!dir.cd(QStringLiteral("Resources")) || !dir.exists(QStringLiteral("docSet.dsidx")))
        return;

    m_db = new Util::SQLiteDatabase(dir.absoluteFilePath(QStringLiteral("docSet.dsidx")));

    if (!m_db->isOpen()) {
        qWarning("SQL Error: %s", qPrintable(m_db->lastError()));
        return;
    }

    sqlite3_create_function(m_db->handle(), "zealScore", 2, SQLITE_UTF8, nullptr, sqliteScoreFunction,
                            nullptr, nullptr);

    m_type = m_db->tables().contains(QStringLiteral("searchIndex")) ? Type::Dash : Type::ZDash;

    createIndex();

    if (!dir.cd(QStringLiteral("Documents"))) {
        m_type = Type::Invalid;
        return;
    }

    // Setup keywords
    if (plist.contains(InfoPlist::DocSetPlatformFamily))
        m_keywords << plist[InfoPlist::DocSetPlatformFamily].toString();

    if (plist.contains(InfoPlist::DashDocSetPluginKeyword))
        m_keywords << plist[InfoPlist::DashDocSetPluginKeyword].toString();

    if (plist.contains(InfoPlist::DashDocSetKeyword))
        m_keywords << plist[InfoPlist::DashDocSetKeyword].toString();

    if (plist.contains(InfoPlist::DashDocSetFamily)) {
        const QString kw = plist[InfoPlist::DashDocSetFamily].toString();
        if (kw != QLatin1String("dashtoc") && kw != QLatin1String("unsorteddashtoc"))
            m_keywords << kw;
    }

    m_keywords.removeDuplicates();

    // Prefer index path provided by the docset over metadata.
    if (plist.contains(InfoPlist::DashIndexFilePath)) {
        m_indexFileUrl = createPageUrl(plist[InfoPlist::DashIndexFilePath].toString());
    } else if (m_indexFileUrl.isEmpty()) {
        if (dir.exists(QStringLiteral("index.html")))
            m_indexFileUrl = createPageUrl(QStringLiteral("index.html"));
        else
            qWarning("Cannot determine index file for docset %s", qPrintable(m_name));
    }

    countSymbols();
}

Docset::~Docset()
{
    delete m_db;
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

QIcon Docset::symbolTypeIcon(const QString &symbolType) const
{
    static const QIcon unknownIcon(QStringLiteral("typeIcon:Unknown.png"));

    const QIcon icon(QStringLiteral("typeIcon:%1.png").arg(symbolType));
    return icon.availableSizes().isEmpty() ? unknownIcon : icon;
}

QUrl Docset::indexFileUrl() const
{
    return m_indexFileUrl;
}

QMap<QString, int> Docset::symbolCounts() const
{
    return m_symbolCounts;
}

int Docset::symbolCount(const QString &symbolType) const
{
    return m_symbolCounts.value(symbolType);
}

const QMap<QString, QUrl> &Docset::symbols(const QString &symbolType) const
{
    if (!m_symbols.contains(symbolType))
        loadSymbols(symbolType);
    return m_symbols[symbolType];
}

QList<SearchResult> Docset::search(const QString &query, const CancellationToken &token) const
{
    // Make it safe to use in a SQL query.
    QString sanitizedQuery = query;
    sanitizedQuery.replace(QLatin1Char('\''), QLatin1String("''"));

    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT name, type, path, '', zealScore('%1', name) as score "
                                  "    FROM searchIndex "
                                  "WHERE score > 0 "
                                  "ORDER BY score DESC").arg(sanitizedQuery);
    } else {
        queryStr = QStringLiteral("SELECT ztokenname, ztypename, zpath, zanchor, zealScore('%1', ztokenname) as score "
                                  "    FROM ztoken "
                                  "LEFT JOIN ztokenmetainformation "
                                  "    ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "LEFT JOIN zfilepath "
                                  "    ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "LEFT JOIN ztokentype "
                                  "    ON ztoken.ztokentype = ztokentype.z_pk "
                                  "WHERE score > 0 "
                                  "ORDER BY score DESC").arg(sanitizedQuery);
    }

    // Limit for very short queries.
    // TODO: Show a notification about the reduced result set.
    if (query.size() < 3)
        queryStr += QLatin1String(" LIMIT 1000");

    QList<SearchResult> results;

    m_db->execute(queryStr);
    while (m_db->next() && !token.isCanceled()) {
        results.append({m_db->value(0).toString(),
                        parseSymbolType(m_db->value(1).toString()),
                        const_cast<Docset *>(this),
                        createPageUrl(m_db->value(2).toString(), m_db->value(3).toString()),
                        m_db->value(4).toInt()});
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
                                  "LEFT JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "LEFT JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "LEFT JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk "
                                  "WHERE zfilepath.zpath = \"%1\" AND ztokenmetainformation.zanchor IS NOT NULL");
    }

    m_db->execute(queryStr.arg(cleanUrl.toString()));
    while (m_db->next()) {
        results.append({m_db->value(0).toString(),
                        parseSymbolType(m_db->value(1).toString()),
                        const_cast<Docset *>(this),
                        createPageUrl(m_db->value(2).toString(), m_db->value(3).toString()),
                        0});
    }

    if (results.size() == 1)
        results.clear();

    return results;
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

        if (extra.contains(QStringLiteral("indexFilePath"))) {
            m_indexFileUrl = createPageUrl(extra[QStringLiteral("indexFilePath")].toString());
        }

        if (extra.contains(QStringLiteral("keywords"))) {
            for (const QJsonValue &kw : extra[QStringLiteral("keywords")].toArray())
                m_keywords << kw.toString();
        }
    }
}

void Docset::countSymbols()
{
    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT type, COUNT(*) FROM searchIndex GROUP BY type");
    } else if (m_type == Docset::Type::ZDash) {
        queryStr = QStringLiteral("SELECT ztypename, COUNT(*) FROM ztoken LEFT JOIN ztokentype"
                                  " ON ztoken.ztokentype = ztokentype.z_pk GROUP BY ztypename");
    }

    if (!m_db->execute(queryStr)) {
        qWarning("SQL Error: %s", qPrintable(m_db->lastError()));
        return;
    }

    while (m_db->next()) {
        const QString symbolTypeStr = m_db->value(0).toString();
        const QString symbolType = parseSymbolType(symbolTypeStr);
        m_symbolStrings.insertMulti(symbolType, symbolTypeStr);
        m_symbolCounts[symbolType] += m_db->value(1).toInt();
    }
}

// TODO: Fetch and cache only portions of symbols
void Docset::loadSymbols(const QString &symbolType) const
{
    for (const QString &symbol : m_symbolStrings.values(symbolType))
        loadSymbols(symbolType, symbol);
}

void Docset::loadSymbols(const QString &symbolType, const QString &symbolString) const
{
    QString queryStr;
    if (m_type == Docset::Type::Dash) {
        queryStr = QStringLiteral("SELECT name, path FROM searchIndex WHERE type='%1' ORDER BY name ASC");
    } else {
        queryStr = QStringLiteral("SELECT ztokenname, zpath, zanchor "
                                  "FROM ztoken "
                                  "LEFT JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                                  "LEFT JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                                  "LEFT JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk WHERE ztypename='%1' "
                                  "ORDER BY ztokenname ASC");
    }

    if (!m_db->execute(queryStr.arg(symbolString))) {
        qWarning("SQL Error: %s", qPrintable(m_db->lastError()));
        return;
    }

    QMap<QString, QUrl> &symbols = m_symbols[symbolType];
    while (m_db->next())
        symbols.insertMulti(m_db->value(0).toString(),
                            createPageUrl(m_db->value(1).toString(), m_db->value(2).toString()));
}

void Docset::createIndex()
{
    static const QString indexListQuery = QStringLiteral("PRAGMA INDEX_LIST('%1')");
    static const QString indexDropQuery = QStringLiteral("DROP INDEX '%1'");
    static const QString indexCreateQuery = QStringLiteral("CREATE INDEX IF NOT EXISTS %1%2"
                                                           " ON %3 (%4 COLLATE NOCASE)");

    const QString tableName = m_type == Type::Dash ? QStringLiteral("searchIndex")
                                                   : QStringLiteral("ztoken");
    const QString columnName = m_type == Type::Dash ? QStringLiteral("name")
                                                    : QStringLiteral("ztokenname");

    m_db->execute(indexListQuery.arg(tableName));

    QStringList oldIndexes;

    while (m_db->next()) {
        const QString indexName = m_db->value(1).toString();
        if (!indexName.startsWith(IndexNamePrefix))
            continue;

        if (indexName.endsWith(IndexNameVersion))
            return;

        oldIndexes << indexName;
    }

    // Drop old indexes
    for (const QString &oldIndexName : oldIndexes)
        m_db->execute(indexDropQuery.arg(oldIndexName));

    m_db->execute(indexCreateQuery.arg(IndexNamePrefix, IndexNameVersion, tableName, columnName));
}

QUrl Docset::createPageUrl(const QString &path, const QString &fragment) const
{
    QString realPath;
    QString realFragment;

    if (fragment.isEmpty()) {
        const QStringList urlParts = path.split(QLatin1Char('#'));
        realPath = urlParts[0];
        if (urlParts.size() > 1)
            realFragment = urlParts[1];
    } else {
        realPath = path;
        realFragment = fragment;
    }

    static const QRegularExpression dashEntryRegExp(QLatin1String("<dash_entry_.*>"));
    realPath.remove(dashEntryRegExp);
    realFragment.remove(dashEntryRegExp);

    QUrl url = QUrl::fromLocalFile(QDir(documentPath()).absoluteFilePath(realPath));
    if (!realFragment.isEmpty()) {
        if (realFragment.startsWith(QLatin1String("//apple_ref"))
                || realFragment.startsWith(QLatin1String("//dash_ref"))) {
            url.setFragment(realFragment, QUrl::DecodedMode);
        } else {
            url.setFragment(realFragment);
        }
    }

    return url;
}

QString Docset::parseSymbolType(const QString &str)
{
    // Dash symbol aliases
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
        {QStringLiteral("enumdata"), QStringLiteral("Constant")},
        {QStringLiteral("enumelt"), QStringLiteral("Constant")},
        {QStringLiteral("clconst"), QStringLiteral("Constant")},
        {QStringLiteral("structdata"), QStringLiteral("Constant")},
        {QStringLiteral("writerid"), QStringLiteral("Constant")},
        {QStringLiteral("Notifications"), QStringLiteral("Constant")},
        // Constructor
        {QStringLiteral("structctr"), QStringLiteral("Constructor")},
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
        {QStringLiteral("enumcm"), QStringLiteral("Method")},
        {QStringLiteral("enumctr"), QStringLiteral("Method")},
        {QStringLiteral("enumm"), QStringLiteral("Method")},
        {QStringLiteral("intfctr"), QStringLiteral("Method")},
        {QStringLiteral("intfcm"), QStringLiteral("Method")},
        {QStringLiteral("intfm"), QStringLiteral("Method")},
        {QStringLiteral("intfsub"), QStringLiteral("Method")},
        {QStringLiteral("instsub"), QStringLiteral("Method")},
        {QStringLiteral("instctr"), QStringLiteral("Method")},
        {QStringLiteral("instm"), QStringLiteral("Method")},
        {QStringLiteral("structcm"), QStringLiteral("Method")},
        {QStringLiteral("structm"), QStringLiteral("Method")},
        {QStringLiteral("structsub"), QStringLiteral("Method")},
        {QStringLiteral("Class Methods"), QStringLiteral("Method")},
        {QStringLiteral("Inherited Methods"), QStringLiteral("Method")},
        {QStringLiteral("Instance Methods"), QStringLiteral("Method")},
        {QStringLiteral("Private Methods"), QStringLiteral("Method")},
        {QStringLiteral("Protected Methods"), QStringLiteral("Method")},
        {QStringLiteral("Public Methods"), QStringLiteral("Method")},
        // Operator
        {QStringLiteral("intfopfunc"), QStringLiteral("Operator")},
        {QStringLiteral("opfunc"), QStringLiteral("Operator")},
        // Property
        {QStringLiteral("enump"), QStringLiteral("Property")},
        {QStringLiteral("intfdata"), QStringLiteral("Property")},
        {QStringLiteral("intfp"), QStringLiteral("Property")},
        {QStringLiteral("instp"), QStringLiteral("Property")},
        {QStringLiteral("structp"), QStringLiteral("Property")},
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

// Based on https://github.com/bevacqua/fuzzysearch
static void matchFuzzy(const char *needle, int needleLength,
                       const char *haystack, int haystackLength,
                       int *start, int *length)
{
    static const int MaxDistance = 8;
    static const int MaxGroupCount = 3;

    *start = -1;

    int groupCount = 0;

    for (int i = 0, j = 0; i < needleLength; ++i) {
        bool found = false;
        bool first = true;
        int distance = 0;

        while (j < haystackLength) {
            if (needle[i] == haystack[j++]) {
                if (*start == -1)
                    *start = j;  // first matched char

                *length = j - *start + 1;
                found = true;
                break;
            }

            // Optimizations to reduce returned number of results
            // (search was returning too many irrelevant results with large docsets)
            // Optimization #1: too many mismatches.
            if (first) {
                if (++groupCount >= MaxGroupCount) {
                    break;
                }

                first = false;
            }

            // Optimization #2: too large distance between found chars.
            if (i != 0 && ++distance >= MaxDistance) {
                break;
            }
        }

        if (!found) {
            // End of haystack, char not found.
            *start = -1;
            return;
        }
    }
}

// Ported from DevDocs (https://github.com/Thibaut/devdocs), see app/searcher.coffee.
static int scoreExact(int matchIndex, int matchLen, const char *value, int valueLen)
{
    static const char DOT = '.';

    int score = 100;

    // Remove one point for each unmatched character.
    score -= (valueLen - matchLen);

    if (matchIndex > 0) {
        if (value[matchIndex - 1] == DOT) {
            // If the character preceding the query is a dot, assign the same
            // score as if the query was found at the beginning of the string,
            // minus one.
            score += matchIndex - 1;
        } else if (matchLen == 1) {
            // Don't match a single-character query unless it's found at the
            // beginning of the string or is preceded by a dot.
            return 0;
        } else {
            // (1) Remove one point for each unmatched character up to
            //     the nearest preceding dot or the beginning of the
            //     string.
            // (2) Remove one point for each unmatched character
            //     following the query.
            int i = matchIndex - 2;
            while (i >= 0 && value[i] != DOT)
                --i;

            score -= (matchIndex - i)                      // (1)
                    + (valueLen - matchLen - matchIndex);  // (2)
        }

        // Remove one point for each dot preceding the query, except for the
        // one immediately before the query.
        for (int i = matchIndex - 2; i >= 0; --i) {
            if (value[i] == DOT)
                --score;
        }
    }

    // Remove five points for each dot following the query.
    for (int i = valueLen - matchLen - matchIndex - 1; i >= 0; --i) {
        if (value[matchIndex + matchLen + i] == DOT)
            score -= 5;
    }

    return qMax(1, score);
}

/**
 * \brief Returns score based on a substring position in a string.
 * \param str Original string.
 * \param index Index of the substring within \a str.
 * \param length Substring length.
 * \return Score value between 1 and 100.
 */
static int scoreFuzzy(const char *str, int index, int length)
{
    if (index == 0 || str[index - 1] == '.') {
        // score between 66..99, if the match follows a dot, or starts the string
        return qMax(66, 100 - length);
    } else if (str[index + length] == 0) {
        // score between 33..66, if the match is at the end of the string
        return qMax(33, 67 - length);
    } else {
        // score between 1..33 otherwise (match in the middle of the string)
        return qMax(1, 34 - length);
    }
}

static void sqliteScoreFunction(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    Q_UNUSED(argc);

    const char *needleOrig = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    const char *haystackOrig = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));

    const int needleLength = static_cast<int>(qstrlen(needleOrig));
    const int haystackLength = static_cast<int>(qstrlen(haystackOrig));

    QScopedArrayPointer<char> needle(new char[needleLength + 1]);
    QScopedArrayPointer<char> haystack(new char[haystackLength + 1]);

    for (int i = 0, j = 0; i <= needleLength; ++i, ++j) {
        const char c = needleOrig[i];
        if ((i > 0 && needleOrig[i - 1] == ':' && c == ':') // C++ (::)
                || c == '/' || c == '_' || c == ' ') { // Go, some Guides
            needle[j] = '.';
        } else if (c >= 'A' && c <= 'Z')  {
            needle[j] = c + 32;
        } else {
            needle[j] = c;
        }
    }

    for (int i = 0, j = 0; i <= haystackLength; ++i, ++j) {
        const char c = haystackOrig[i];
        if ((i > 0 && haystackOrig[i - 1] == ':' && c == ':') // C++ (::)
                || c == '/' || c == '_' || c == ' ') { // Go, some Guides
            haystack[j] = '.';
        } else if (c >= 'A' && c <= 'Z')  {
            haystack[j] = c + 32;
        } else {
            haystack[j] = c;
        }
    }

    int score = 0;
    int matchIndex = -1;
    int matchLength;

    matchFuzzy(needle.data(), needleLength,
               haystack.data(), haystackLength,
               &matchIndex, &matchLength);

    if (matchIndex == -1) { // no match
        // simply return 0
        sqlite3_result_int(context, 0);
        return;
    } else if (needleLength == matchLength) {
        // +100 to make sure exact matches are always on top.
        score = scoreExact(matchIndex, matchLength, haystack.data(), haystackLength) + 100;
    } else {
        score = scoreFuzzy(haystack.data(), matchIndex, matchLength);

        int indexOfLastDot;
        for (indexOfLastDot = haystackLength - 1; indexOfLastDot >= 0; --indexOfLastDot) {
            if (haystack[indexOfLastDot] == '.')
                break;
        }

        if (indexOfLastDot != -1) {
            matchIndex = -1;
            matchFuzzy(needle.data(), needleLength,
                       haystack.data() + indexOfLastDot + 1, haystackLength - (indexOfLastDot + 1),
                       &matchIndex, &matchLength);

            if (matchIndex != -1) {
                score = qMax(score, scoreFuzzy(haystack.data() + indexOfLastDot + 1,
                                               matchIndex, matchLength));
            }
        }
    }

    sqlite3_result_int(context, score);
}
