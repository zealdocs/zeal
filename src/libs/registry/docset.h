// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_DOCSET_H
#define ZEAL_REGISTRY_DOCSET_H

#include <QIcon>
#include <QMap>
#include <QMetaObject>
#include <QMultiMap>
#include <QUrl>

namespace Zeal {

namespace Util {
class SQLiteDatabase;
}

namespace Registry {

class CancellationToken;
struct SearchResult;

class Docset final
{
    Q_DISABLE_COPY_MOVE(Docset)
public:
    explicit Docset(QString path);
    virtual ~Docset();

    bool isValid() const;

    QString name() const;
    QString title() const;
    QStringList keywords() const;

    QString version() const;
    int revision() const;
    QString feedUrl() const;

    QString path() const;
    QString documentPath() const;
    QIcon icon() const;
    QIcon symbolTypeIcon(const QString &symbolType) const;
    QUrl indexFileUrl() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QMultiMap<QString, QUrl> &symbols(const QString &symbolType) const;

    QList<SearchResult> search(const QString &query, const CancellationToken &token) const;
    QList<SearchResult> relatedLinks(const QUrl &url) const;

    // FIXME: This a temporary solution to create URL on demand.
    QUrl searchResultUrl(const SearchResult &result) const;

    // FIXME: This is an ugly workaround before we have a proper docset sources implementation
    bool hasUpdate = false;

    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &baseUrl);

    bool isFuzzySearchEnabled() const;
    void setFuzzySearchEnabled(bool enabled);

    bool isJavaScriptEnabled() const;

private:
    enum class Type {
        Invalid,
        Dash,
        ZDash
    };

    void loadMetadata();
    void countSymbols();
    void loadSymbols(const QString &symbolType) const;
    void loadSymbols(const QString &symbolType, const QString &symbolString) const;
    void createIndex();
    void createView();
    QUrl createPageUrl(const QString &path, const QString &fragment = QString()) const;

    static QString parseSymbolType(const QString &str);

    QString m_name;
    QString m_title;
    QStringList m_keywords;
    QString m_version;
    int m_revision = 0;
    QString m_feedUrl;
    Docset::Type m_type = Type::Invalid;
    QString m_path;
    QIcon m_icon;

    QUrl m_indexFileUrl;
    QString m_indexFilePath;

    QMultiMap<QString, QString> m_symbolStrings;
    QMap<QString, int> m_symbolCounts;
    mutable QMap<QString, QMultiMap<QString, QUrl>> m_symbols;
    Util::SQLiteDatabase *m_db = nullptr;

    bool m_isFuzzySearchEnabled = false;
    bool m_isJavaScriptEnabled = false;

    QUrl m_baseUrl;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_DOCSET_H
