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
    QIcon icon() const;

    QString prefix;
    Type type;
    QString documentPath;
    QSqlDatabase db;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    bool m_isValid = false;

    QString m_name;
};

} // namespace Zeal

#endif // DOCSET_H
