/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "plist.h"

#include <QFile>
#include <QXmlStreamReader>

using namespace Zeal::Util;

Plist::Plist() :
    QHash<QString, QVariant>()
{
}

bool Plist::read(const QString &fileName)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    if (!file->open(QIODevice::ReadOnly)) {
        // TODO: Report/log error
        m_hasError = true;
        return false;
    }
    
    char buf;
    // Skip whitespace at the beginning of file
    while (file->peek(&buf, 1) > 0 && isspace(buf))
        file->read(1);

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

    return m_hasError;
}

bool Plist::hasError() const
{
    return m_hasError;
}
