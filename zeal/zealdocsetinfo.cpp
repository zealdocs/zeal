#include "zealdocsetinfo.h"

#include <QDomDocument>
#include <QFile>

ZealDocsetInfo::ZealDocsetInfo(const QString &filePath)
{
    if (QFile::exists(filePath))
        readDocset(filePath);
}

bool ZealDocsetInfo::readDocset(const QString &filePath)
{
    QScopedPointer<QFile> file(new QFile(filePath));
    if (!file->open(QIODevice::ReadOnly))
        return false;

    QDomDocument infoplist(QStringLiteral("infoplist"));
    if (!infoplist.setContent(file.data()))
        return false;

    const QDomNodeList keys = infoplist.elementsByTagName(QStringLiteral("key"));
    for (int i = 0; i < keys.count(); ++i) {
        const QDomNode key = keys.at(i);
        const QString nodeValue = key.firstChild().nodeValue();

        if (nodeValue == QStringLiteral("dashIndexFilePath"))
            indexPath = key.nextSibling().firstChild().nodeValue();
        else if (nodeValue == QStringLiteral("DashDocSetKeyword"))
            keyword = key.nextSibling().firstChild().nodeValue();
        else if (nodeValue == QStringLiteral("DashDocSetFamily"))
            family = key.nextSibling().firstChild().nodeValue();
        else if (nodeValue == QStringLiteral("CFBundleName"))
            bundleName = key.nextSibling().firstChild().nodeValue();
        else if (nodeValue == QStringLiteral("CFBundleIdentifier"))
            bundleIdentifier = key.nextSibling().firstChild().nodeValue();
    }
    return true;
}
