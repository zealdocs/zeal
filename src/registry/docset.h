#ifndef DOCSET_H
#define DOCSET_H

#include "docsetinfo.h"
#include "docsetmetadata.h"

#include <QIcon>
#include <QMap>
#include <QMetaObject>
#include <QString>
#include <QSqlDatabase>

namespace Zeal {

class Docset : public QObject
{
    Q_GADGET
    Q_ENUMS(SymbolType)
public:
    enum class Type {
        Dash,
        ZDash
    };

    enum class SymbolType {
        Invalid, // Internal use only
        Attribute,
        Class,
        Command,
        Constant,
        Constructor,
        Conversion,
        Delegate,
        Directive,
        Enumeration,
        Event,
        Exception,
        Field,
        Filter,
        Function,
        Guide,
        Interface,
        Macro,
        Method,
        Module,
        Namespace,
        Object,
        Operator,
        Option,
        Package,
        Property,
        Setting,
        Specialization,
        Structure,
        Tag,
        Trait,
        Type,
        Variable
    };

    explicit Docset(const QString &path);
    ~Docset();

    bool isValid() const;

    QString name() const;
    Docset::Type type() const;
    QString path() const;
    QString documentPath() const;
    QIcon icon() const;

    QMap<SymbolType, int> symbolCounts() const;
    QList<Docset::SymbolType> symbolTypes() const;
    int symbolCount(Docset::SymbolType type) const;
    int symbolCount(const QString &typeStr) const;

    const QMap<QString, QString> &symbols(Docset::SymbolType type) const;

    static QString symbolTypeToStr(SymbolType symbolType);
    static SymbolType strToSymbolType(const QString &str);

    QString prefix;
    QSqlDatabase db;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    void findIcon();
    void countSymbols();
    void loadSymbols(SymbolType symbolType) const;

    bool m_isValid = false;

    QString m_name;
    Docset::Type m_type;
    QString m_path;
    QIcon m_icon;

    QMap<SymbolType, QString> m_symbolStrings;
    QMap<SymbolType, int> m_symbolCounts;
    mutable QMap<SymbolType, QMap<QString, QString>> m_symbols;
};

} // namespace Zeal

#endif // DOCSET_H
