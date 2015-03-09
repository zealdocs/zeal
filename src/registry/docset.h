#ifndef DOCSET_H
#define DOCSET_H

#include "docsetinfo.h"
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
    enum class Type {
        Dash,
        ZDash
    };

    explicit Docset(const QString &path);
    ~Docset() override;

    bool isValid() const;
    bool hasMetadata() const;

    QString name() const;
    QString title() const;

    QString version() const;
    QString revision() const;

    Docset::Type type() const;
    QString path() const;
    QString documentPath() const;
    QIcon icon() const;
    QString indexFilePath() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QMap<QString, QString> &symbols(const QString &symbolType) const;

    QList<SearchResult> relatedLinks(const QUrl &url) const;

    QSqlDatabase database() const;

    QString prefix;

    /// FIXME: Get rid of it
    static void normalizeName(QString &name, QString &parentName);

private:
    void loadMetadata();
    void findIcon();
    void countSymbols();
    void loadSymbols(const QString &symbolType) const;
    void loadSymbols(const QString &symbolType, const QString &symbolString) const;

    static QString parseSymbolType(const QString &str);

    bool m_isValid = false;
    bool m_hasMetadata = false;

    QString m_sourceId;
    QString m_name;
    QString m_title;
    QString m_version;
    QString m_revision;
    Docset::Type m_type;
    QString m_path;
    QIcon m_icon;

    DocsetInfo m_info;

    QMap<QString, QString> m_symbolStrings;
    QMap<QString, int> m_symbolCounts;
    mutable QMap<QString, QMap<QString, QString>> m_symbols;
};

} // namespace Zeal

#endif // DOCSET_H
