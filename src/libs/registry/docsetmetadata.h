// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_DOCSETMETADATA_H
#define ZEAL_REGISTRY_DOCSETMETADATA_H

#include <QIcon>
#include <QJsonObject>
#include <QStringList>
#include <QUrl>

namespace Zeal {
namespace Registry {

class DocsetMetadata
{
public:
    explicit DocsetMetadata() = default;
    explicit DocsetMetadata(const QJsonObject &jsonObject);

    void save(const QString &path, const QString &version);

    QString name() const;
    QString title() const;
    QStringList aliases() const;
    QStringList versions() const;
    QString latestVersion() const;
    int revision() const;
    QIcon icon() const;

    QUrl feedUrl() const;
    QUrl url() const;
    QList<QUrl> urls() const;

    static DocsetMetadata fromDashFeed(const QUrl &feedUrl, const QByteArray &data);

private:
    QString m_name;
    QString m_title;
    QStringList m_aliases;
    QStringList m_versions;
    int m_revision = 0;

    QByteArray m_rawIcon;
    QByteArray m_rawIcon2x;
    QIcon m_icon;

    QJsonObject m_extra;

    QUrl m_feedUrl;
    QList<QUrl> m_urls;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_DOCSETMETADATA_H
