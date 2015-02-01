#ifndef DOCSET_H
#define DOCSET_H

#include "docsetinfo.h"
#include "docsetmetadata.h"

#include <QIcon>
#include <QString>
#include <QSqlDatabase>

namespace Zeal {

class Docset
{
public:
    enum class Type {
        Dash,
        ZDash
    };

    explicit Docset();
    explicit Docset(const QString &path);
    ~Docset();

    bool isValid() const;

    QString name() const;
    Docset::Type type() const;
    QString path() const;
    QString documentPath() const;
    QIcon icon() const;

    QString prefix;
    QSqlDatabase db;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    void findIcon();

    bool m_isValid = false;

    QString m_name;
    Docset::Type m_type;
    QString m_path;
    QIcon m_icon;
};

} // namespace Zeal

#endif // DOCSET_H
