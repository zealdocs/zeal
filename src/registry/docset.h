#ifndef DOCSET_H
#define DOCSET_H

#include "searchresult.h"

#include <QIcon>
#include <QMap>
#include <QMetaObject>
#include <QSqlDatabase>

namespace Zeal {

class Docset : public QObject
{
    Q_OBJECT
public:
    explicit Docset(const QString &path);
    ~Docset() override;

    bool isValid() const;

    QString name() const;
    QString title() const;
    QString keyword() const;

    QString version() const;
    QString revision() const;

    QString path() const;
    QString documentPath() const;
    QIcon icon() const;
    QString indexFilePath() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QMap<QString, QString> &symbols(const QString &symbolType) const;

    QList<SearchResult> search(const QString &query) const;
    QList<SearchResult> relatedLinks(const QUrl &url) const;

    /// FIXME: This is an ugly workaround before we have a proper docset sources implementation
    bool hasUpdate = false;

private:
    enum class Type {
        Invalid,
        Dash,
        ZDash
    };

    QSqlDatabase database() const;
    void loadMetadata();
    void countSymbols();
    void loadSymbols(const QString &symbolType) const;
    void loadSymbols(const QString &symbolType, const QString &symbolString) const;

    static QString parseSymbolType(const QString &str);

    QString m_sourceId;
    QString m_name;
    QString m_title;
    QString m_keyword;
    QString m_version;
    QString m_revision;
    Docset::Type m_type = Type::Invalid;
    QString m_path;
    QIcon m_icon;

    QString m_indexFilePath;

    QMap<QString, QString> m_symbolStrings;
    QMap<QString, int> m_symbolCounts;
    mutable QMap<QString, QMap<QString, QString>> m_symbols;
};

} // namespace Zeal

#endif // DOCSET_H
