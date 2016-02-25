#ifndef DOCSETTOKEN_H
#define DOCSETTOKEN_H

#include <QString>

namespace Zeal {

class DocsetToken
{
public:
    DocsetToken(const QString &token);
    QString full;
    QString parentName;
    QString separator;
    QString name;
    QString args;

private:
    void parseArgs(const QString &token);
};

} // namespace Zeal

#endif // DOCSETTOKEN_H
