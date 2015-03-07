#ifndef DOCSETMETADATA_H
#define DOCSETMETADATA_H

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

    void toFile(const QString &fileName) const;
    QByteArray toJson() const;

    QString name() const;
    QString icon() const;
    QString title() const;
    QStringList aliases() const;
    QString version() const;
    QString revision() const;
    QStringList oldVersions() const;

    QUrl feedUrl() const;
    QUrl url() const;
    QList<QUrl> urls() const;

    static DocsetMetadata fromFile(const QString &fileName);
    static DocsetMetadata fromDashFeed(const QUrl &feedUrl, const QByteArray &data);

private:
    QString m_sourceId;

    QString m_name;
    QString m_icon;
    QString m_title;
    QStringList m_aliases;
    QString m_version;
    QString m_revision;
    QStringList m_oldVersions;

    QUrl m_feedUrl;
    QList<QUrl> m_urls;
};

} // namespace Zeal

Q_DECLARE_METATYPE(Zeal::DocsetMetadata)

#endif // DOCSETMETADATA_H
