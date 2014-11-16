#ifndef ZEALDOCSETINFO_H
#define ZEALDOCSETINFO_H

#include <QString>

struct ZealDocsetInfo
{
    explicit ZealDocsetInfo(const QString &filePath = QString());
    bool readDocset(const QString &filePath);

    QString indexPath;
    QString family;
    QString keyword;
    QString bundleName;
    QString bundleIdentifier;
};

#endif // ZEALDOCSETINFO_H
