#include "docsetinfo.h"

#include <QFile>
#include <QVariant>
#include <QXmlStreamReader>

using namespace Zeal;

DocsetInfo DocsetInfo::fromPlist(const QString &filePath)
{
    DocsetInfo docsetInfo;

    QScopedPointer<QFile> file(new QFile(filePath));
    if (!file->open(QIODevice::ReadOnly))
        return docsetInfo;

    QXmlStreamReader xml(file.data());

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument || token != QXmlStreamReader::StartElement)
            continue;

        if (xml.name() != QLatin1String("key"))
            continue;

        const QString key = xml.readElementText();

        // Skip whitespaces between tags
        while (xml.readNext() == QXmlStreamReader::Characters);

        if (xml.tokenType() != QXmlStreamReader::StartElement)
            continue;

        QVariant value;
        if (xml.name() == QLatin1String("string"))
            value = xml.readElementText();
        else if (xml.name() == QLatin1String("true"))
            value = true;
        else
            continue; // Skip unknown types

        if (key == QLatin1String("dashIndexFilePath"))
            docsetInfo.indexPath = value.toString();
        else if (key == QLatin1String("DashDocSetKeyword"))
            docsetInfo.keyword = value.toString();
        else if (key == QLatin1String("DashDocSetFamily"))
            docsetInfo.family = value.toString();
        else if (key == QLatin1String("CFBundleName"))
            docsetInfo.bundleName = value.toString();
        else if (key == QLatin1String("CFBundleIdentifier"))
            docsetInfo.bundleIdentifier = value.toString();
        else if (key == QLatin1String("isDashDocset"))
            docsetInfo.isDashDocset = value.toBool();
        else if (key == QLatin1String("isJavaScriptEnabled"))
            docsetInfo.isJavaScriptEnabled = value.toBool();
    }

    /// TODO: Check xml.hasError()

    return docsetInfo;
}
