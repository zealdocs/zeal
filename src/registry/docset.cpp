#include "docset.h"

#include <QDir>

using namespace Zeal;

Docset::Docset()
{

}

Docset::~Docset()
{

}

QIcon Docset::icon() const
{
    const QDir dir(documentPath);

    QIcon icon(dir.absoluteFilePath("favicon.ico"));

    if (icon.availableSizes().isEmpty())
        icon = QIcon(dir.absoluteFilePath("icon.png"));

    if (icon.availableSizes().isEmpty()) {
        QString bundleName = info.bundleName;
        bundleName.replace(" ", "_");

        icon = QIcon(QString("icons:%1.png").arg(bundleName));

        // Fallback to identifier and docset file name.
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(info.bundleIdentifier));
        if (icon.availableSizes().isEmpty())
            icon = QIcon(QString("icons:%1.png").arg(name));
    }
    return icon;
}

