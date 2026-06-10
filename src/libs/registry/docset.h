// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_DOCSET_H
#define ZEAL_REGISTRY_DOCSET_H

#include <QIcon>
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QMultiMap>
#include <QUrl>

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

namespace Zeal {

namespace Util {
class Database;
class TarixArchive;
} // namespace Util

namespace Registry {

struct SearchResult;

class Docset final
{
    Q_DISABLE_COPY_MOVE(Docset)
public:
    explicit Docset(QString path);
    ~Docset();

    bool isValid() const;

    QString name() const;
    QString title() const;
    QStringList keywords() const;

    QString version() const;
    int revision() const;
    QString feedUrl() const;

    QString path() const;
    QString documentPath() const;

    bool isArchived() const;
    std::optional<QByteArray> readDocument(const QString &path) const;

    QIcon icon() const;
    static QIcon symbolTypeIcon(const QString &symbolType);
    QUrl indexFileUrl() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QList<std::pair<QString, QUrl>> &symbols(const QString &symbolType) const;

    QList<SearchResult> search(const QString &query, const std::atomic_bool &canceled) const;
    QList<SearchResult> relatedLinks(const QUrl &url) const;

    // Update availability lives here until a proper docset catalog implementation exists.
    struct UpdateInfo
    {
        QString version;
        int revision = 0;
        qint64 size = 0; // Compressed archive size in bytes; 0 when unknown (e.g. user dash feeds).
    };

    bool hasUpdate() const;
    const std::optional<UpdateInfo> &update() const;
    void setUpdate(std::optional<UpdateInfo> update);

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
    mutable QMap<QString, QList<std::pair<QString, QUrl>>> m_symbols;
    Util::Database *m_db = nullptr;
    std::unique_ptr<Util::TarixArchive> m_tarixArchive;

    bool m_isFuzzySearchEnabled = false;
    bool m_isJavaScriptEnabled = false;

    QUrl m_baseUrl;

    std::optional<UpdateInfo> m_update;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_DOCSET_H
