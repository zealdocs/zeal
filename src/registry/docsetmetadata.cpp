#include "docsetmetadata.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>

using namespace Zeal;

DocsetMetadata::DocsetMetadata()
{
}

DocsetMetadata::DocsetMetadata(const QJsonObject &jsonObject)
{
    m_sourceId = jsonObject[QStringLiteral("sourceId")].toString();

    m_name = jsonObject[QStringLiteral("name")].toString();
    m_icon = jsonObject[QStringLiteral("icon")].toString();
    m_title = jsonObject[QStringLiteral("title")].toString();
    m_version = jsonObject[QStringLiteral("version")].toString();
    m_revision = jsonObject[QStringLiteral("revision")].toString();

    for (const QJsonValue &vv : jsonObject[QStringLiteral("aliases")].toArray())
        m_aliases << vv.toString();

    for (const QJsonValue &vv : jsonObject[QStringLiteral("oldVersions")].toArray())
        m_oldVersions << vv.toString();

    m_feedUrl = jsonObject[QStringLiteral("feed_url")].toString();
    const QJsonArray urlArray = jsonObject[QStringLiteral("urls")].toArray();
    for (const QJsonValue &url : urlArray)
        m_urls.append(url.toString());
}

QString DocsetMetadata::sourceId() const
{
    return m_sourceId;
}

void DocsetMetadata::toFile(const QString &fileName) const
{
    QScopedPointer<QFile> file(new QFile(fileName));
    if (!file->open(QIODevice::WriteOnly))
        return;

    file->write(toJson());
}

QByteArray DocsetMetadata::toJson() const
{
    QJsonObject jsonObject;

    jsonObject[QStringLiteral("sourceId")] = m_sourceId;

    jsonObject[QStringLiteral("name")] = m_name;
    jsonObject[QStringLiteral("icon")] = m_icon;
    jsonObject[QStringLiteral("title")] = m_title;
    jsonObject[QStringLiteral("version")] = m_version;
    jsonObject[QStringLiteral("revision")] = m_revision;

    QJsonArray aliases;
    for (const QString &alias : m_aliases)
        aliases.append(alias);
    jsonObject[QStringLiteral("aliases")] = aliases;

    QJsonArray oldVersions;
    for (const QString &oldVersion : m_oldVersions)
        oldVersions.append(oldVersion);
    jsonObject[QStringLiteral("oldVersions")] = oldVersions;


    jsonObject[QStringLiteral("feed_url")] = m_feedUrl.toString();

    QJsonArray urls;
    for (const QUrl &url : m_urls)
        urls.append(url.toString());
    jsonObject[QStringLiteral("urls")] = urls;

    return QJsonDocument(jsonObject).toJson();
}

QString DocsetMetadata::name() const
{
    return m_name;
}

QString DocsetMetadata::icon() const
{
    return m_icon;
}

QString DocsetMetadata::title() const
{
    return m_title;
}

QStringList DocsetMetadata::aliases() const
{
    return m_aliases;
}

QString DocsetMetadata::version() const
{
    return m_version;
}

QString DocsetMetadata::revision() const
{
    return m_revision;
}

QStringList DocsetMetadata::oldVersions() const
{
    return m_oldVersions;
}

QUrl DocsetMetadata::feedUrl() const
{
    return m_feedUrl;
}

QUrl DocsetMetadata::url() const
{
    return m_urls.at(qrand() % m_urls.size());
}

QList<QUrl> DocsetMetadata::urls() const
{
    return m_urls;
}

DocsetMetadata DocsetMetadata::fromFile(const QString &fileName)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    if (!file->open(QIODevice::ReadOnly))
        return DocsetMetadata();

    return DocsetMetadata(QJsonDocument::fromJson(file->readAll()).object());
}

DocsetMetadata DocsetMetadata::fromDashFeed(const QUrl &feedUrl, const QByteArray &data)
{
    DocsetMetadata metadata;
    metadata.m_sourceId = QStringLiteral("dash-feed");

    metadata.m_name = feedUrl.fileName();
    metadata.m_name.chop(4); // Strip ".xml" extension

    metadata.m_title = metadata.m_name;
    metadata.m_title.replace(QLatin1Char('_'), QLatin1Char(' '));

    metadata.m_feedUrl = feedUrl;

    QXmlStreamReader xml(data);

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument || token != QXmlStreamReader::StartElement)
            continue;

        // Try to pull out the relevant data
        if (xml.name() == QLatin1String("version")) {
            if (xml.readNext() != QXmlStreamReader::Characters)
                continue;
            metadata.m_version = xml.text().toString();
        } else if (xml.name() == QLatin1String("url")) {
            if (xml.readNext() != QXmlStreamReader::Characters)
                continue;
            metadata.m_urls.append(xml.text().toString());
        }
    }

    return metadata;
}
