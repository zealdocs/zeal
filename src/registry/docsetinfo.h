#ifndef DOCSETINFO_H
#define DOCSETINFO_H

#include <QString>

namespace Zeal {

struct DocsetInfo
{
    static DocsetInfo fromPlist(const QString &filePath);

    QString bundleName;
    QString bundleIdentifier;
    QString indexPath;
    QString family;
    QString keyword;
    bool isDashDocset = false;
    bool isJavaScriptEnabled = false;
};

} // namespace Zeal

#endif // DOCSETINFO_H
