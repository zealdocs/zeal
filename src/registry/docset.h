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
    QIcon icon() const;

    QString name;
    QString prefix;
    Type type;
    QString documentPath;
    QSqlDatabase db;
    DocsetMetadata metadata;
    DocsetInfo info;

private:
    bool m_isValid = false;
};

} // namespace Zeal

#endif // DOCSET_H
