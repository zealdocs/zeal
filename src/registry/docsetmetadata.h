/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef DOCSETMETADATA_H
#define DOCSETMETADATA_H

#include <QJsonObject>
#include <QIcon>
#include <QStringList>
#include <QUrl>

class QJsonObject;

namespace Zeal {

/// TODO: Use QUrl

class DocsetMetadata
{
public:
    explicit DocsetMetadata();
    explicit DocsetMetadata(const QJsonObject &jsonObject);

    QString sourceId() const;

    void save(const QString &path, const QString &version);

    QString name() const;
    QString title() const;
    QStringList aliases() const;
    QStringList versions() const;
    QString latestVersion() const;
    QString revision() const;
    QIcon icon() const;

    QUrl feedUrl() const;
    QUrl url() const;
    QList<QUrl> urls() const;

    static DocsetMetadata fromDashFeed(const QUrl &feedUrl, const QByteArray &data);

private:
    QString m_sourceId;

    QString m_name;
    QString m_title;
    QStringList m_aliases;
    QStringList m_versions;
    QString m_revision;

    QByteArray m_rawIcon;
    QByteArray m_rawIcon2x;
    QIcon m_icon;

    QJsonObject m_extra;

    QUrl m_feedUrl;
    QList<QUrl> m_urls;
};

} // namespace Zeal

#endif // DOCSETMETADATA_H
