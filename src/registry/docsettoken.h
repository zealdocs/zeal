#ifndef DOCSETTOKEN_H
#define DOCSETTOKEN_H

#include <QString>

namespace Zeal {

class DocsetToken
{
public:
    DocsetToken(const QString &token);
    QString full;
    QString name;
    QString parentName;

private:
    QString stripParens(const QString &token) const;
};

} // namespace Zeal

#endif // DOCSETTOKEN_H
