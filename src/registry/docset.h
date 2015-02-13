#ifndef DOCSET_H
#define DOCSET_H

#include "docsetinfo.h"
#include "docsetmetadata.h"

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
    ~Docset();

    bool isValid() const;

    QString name() const;
    Docset::Type type() const;
    QString path() const;
    QString documentPath() const;
    QIcon icon() const;
    QString indexFilePath() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QMap<QString, QString> &symbols(const QString &symbolType) const;

    QSqlDatabase database() const;

    QString prefix;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    void findIcon();
    void countSymbols();
    void loadSymbols(const QString &symbolType) const;
    void loadSymbols(const QString &symbolType, const QString &symbolString) const;

    static QString parseSymbolType(const QString &str);

    bool m_isValid = false;

    QString m_name;
    Docset::Type m_type;
    QString m_path;
    QIcon m_icon;

    QMap<QString, QString> m_symbolStrings;
    QMap<QString, int> m_symbolCounts;
    mutable QMap<QString, QMap<QString, QString>> m_symbols;
};

} // namespace Zeal

#endif // DOCSET_H
