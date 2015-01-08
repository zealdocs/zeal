#ifndef ZEALDOCSETMETADATA_H
#define ZEALDOCSETMETADATA_H

#include <QJsonObject>
#include <QStringList>
#include <QXmlStreamReader>

/// TODO: Use QUrl

class ZealDocsetMetadata
{
public:
    explicit ZealDocsetMetadata();

    void read(QXmlStreamReader &xml);
    void read(const QString &path);

    void write(const QString &path) const;

    bool isValid() const;

    QStringList urls() const;
    QString primaryUrl() const;

    void addUrl(const QString &url);
    int urlCount() const;

    QString version() const;
    void setVersion(const QString &version);

    QString feedUrl() const;
    void setFeedUrl(const QString &url);

private:
    bool m_valid;
    QString m_version;
    QStringList m_urls;
    QString m_feedUrl;
};

Q_DECLARE_METATYPE(ZealDocsetMetadata)

#endif // ZEALDOCSETMETADATA_H
