#include "zealdocsetinfo.h"

#include <QDomDocument>
#include <QFile>

ZealDocsetInfo::ZealDocsetInfo(const QString &filePath)
{
    if (QFile(filePath).exists())
        readDocset(filePath);
}

bool ZealDocsetInfo::readDocset(const QString &filePath)
{
    QFile file(filePath);
    QDomDocument infoplist("infoplist");
    if (!file.open(QIODevice::ReadOnly))
        return false;

    if (!infoplist.setContent(&file)) {
        file.close();
        return false;
    }

    auto keys = infoplist.elementsByTagName("key");
    for (int i = 0; i < keys.count(); ++i) {
        auto key = keys.at(i);
        if (key.firstChild().nodeValue() == "dashIndexFilePath")
            indexPath = key.nextSibling().firstChild().nodeValue();
        else if (key.firstChild().nodeValue() == "DashDocSetKeyword")
            keyword = key.nextSibling().firstChild().nodeValue();
        else if (key.firstChild().nodeValue() == "DashDocSetFamily")
            family = key.nextSibling().firstChild().nodeValue();
        else if (key.firstChild().nodeValue() == "CFBundleName")
            bundleName = key.nextSibling().firstChild().nodeValue();
        else if (key.firstChild().nodeValue() == "CFBundleIdentifier")
            bundleIdentifier = key.nextSibling().firstChild().nodeValue();

    }
    return true;
}
