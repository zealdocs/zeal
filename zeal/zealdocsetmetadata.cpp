#include "zealdocsetmetadata.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

ZealDocsetMetadata::ZealDocsetMetadata() :
    m_valid(true)
{
}

void ZealDocsetMetadata::read(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument || token != QXmlStreamReader::StartElement)
            continue;

        // Try to pull out the relevant data
        if (xml.name() == QStringLiteral("version")) {
            if (xml.readNext() != QXmlStreamReader::Characters)
                continue;
            m_version = xml.text().toString();
        } else if (xml.name() == QStringLiteral("url")) {
            if (xml.readNext() != QXmlStreamReader::Characters)
                continue;
            m_urls.append(xml.text().toString());
        }
    }
}

void ZealDocsetMetadata::read(const QString &path)
{
    QScopedPointer<QFile> file(new QFile(path));
    if (!file->open(QIODevice::ReadOnly)) {
        m_valid = false;
        return;
    }

    const QJsonObject json = QJsonDocument::fromJson(file->readAll()).object();
    m_version = json[QStringLiteral("version")].toString();
    m_feedUrl = json[QStringLiteral("feed_url")].toString();

    const QJsonArray urlArray = json[QStringLiteral("urls")].toArray();
    for (QJsonValue url : urlArray)
        m_urls.append(url.toString());
}

void ZealDocsetMetadata::write(const QString &path) const
{
    QScopedPointer<QFile> file(new QFile(path));
    if (!file->open(QIODevice::WriteOnly))
        return;

    QJsonObject json;
    json[QStringLiteral("version")] = m_version;
    json[QStringLiteral("feed_url")] = m_feedUrl;

    QJsonArray urlArray;
    for (auto url : m_urls)
        urlArray.append(url);
    json[QStringLiteral("urls")] = urlArray;

    const QJsonDocument doc(json);
    file->write(doc.toJson());
}

bool ZealDocsetMetadata::isValid() const
{
    return m_valid;
}

QStringList ZealDocsetMetadata::urls() const
{
    return m_urls;
}

QString ZealDocsetMetadata::primaryUrl() const
{
    // Find preferred mirror from feed url host
    QUrl feedUrl(m_feedUrl);

    const QRegExp regex(QStringLiteral("[^:]+://")
                        + feedUrl.host().replace(QStringLiteral("."), QStringLiteral("\\."))
                        + QStringLiteral(".*"));

    int idx = m_urls.indexOf(regex);

    if (idx < 0)
        idx = qrand() % m_urls.count();

    return m_urls[idx];
}

void ZealDocsetMetadata::addUrl(const QString &url)
{
    m_urls.append(url);
}

int ZealDocsetMetadata::urlCount() const
{
    return m_urls.size();
}

QString ZealDocsetMetadata::version() const
{
    return m_version;
}

void ZealDocsetMetadata::setVersion(const QString &version)
{
    m_version = version;
}

QString ZealDocsetMetadata::feedUrl() const
{
    return m_feedUrl;
}

void ZealDocsetMetadata::setFeedUrl(const QString &url)
{
    m_feedUrl = url;
}
