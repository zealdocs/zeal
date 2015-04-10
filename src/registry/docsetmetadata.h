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
