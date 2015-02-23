#ifndef DOCSETTOKEN_H
#define DOCSETTOKEN_H

#include <QString>

namespace Zeal {

class DocsetToken
{
public:
    DocsetToken(const QString &token);
    QString name() const;
    QString parentName() const;

private:
    QString m_name;
    QString m_parentName;

    QString stripParens(const QString &token) const;
};

} // namespace Zeal

#endif // DOCSETTOKEN_H
