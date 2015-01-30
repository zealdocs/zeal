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
    QString path() const;
    QString documentPath() const;
    QIcon icon() const;

    QString prefix;
    Type type;
    QSqlDatabase db;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    bool m_isValid = false;

    QString m_name;
    QString m_path;
};

} // namespace Zeal

#endif // DOCSET_H
