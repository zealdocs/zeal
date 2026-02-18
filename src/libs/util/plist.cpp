// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plist.h"

#include <QFile>
#include <QLoggingCategory>
#include <QXmlStreamReader>

using namespace Zeal::Util;

static Q_LOGGING_CATEGORY(log, "zeal.util.plist")

bool Plist::read(const QString &fileName)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    if (!file->open(QIODevice::ReadOnly)) {
        qCWarning(log, "Cannot open plist file '%s'.", qPrintable(fileName));
        m_hasError = true;
        return false;
    }

    QXmlStreamReader xml(file.data());

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement)
            continue;

        if (xml.name() != QLatin1String("key"))
            continue; // TODO: Should it fail here?

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
        else if (xml.name() == QLatin1String("false"))
            value = false;
        else
            continue; // Skip unknown types

        insert(key, value);
    }

    return !m_hasError;
}

bool Plist::hasError() const
{
    return m_hasError;
}
