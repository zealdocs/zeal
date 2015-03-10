#include "docsetmetadata.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariant>
#include <QXmlStreamReader>

using namespace Zeal;

DocsetMetadata::DocsetMetadata()
{
}

DocsetMetadata::DocsetMetadata(const QJsonObject &jsonObject)
{
    m_sourceId = jsonObject[QStringLiteral("sourceId")].toString();

    m_name = jsonObject[QStringLiteral("name")].toString();
    m_title = jsonObject[QStringLiteral("title")].toString();

    m_rawIcon = QByteArray::fromBase64(jsonObject[QStringLiteral("icon")].toString().toLocal8Bit());
    m_icon.addPixmap(QPixmap::fromImage(QImage::fromData(m_rawIcon)));

    m_rawIcon2x = QByteArray::fromBase64(jsonObject[QStringLiteral("icon2x")].toString()
            .toLocal8Bit());
    /// TODO: Check on a high-resolution screen
    if (qApp->devicePixelRatio() > 1.0) {
        QPixmap pixmap = QPixmap::fromImage(QImage::fromData(m_rawIcon2x));
        pixmap.setDevicePixelRatio(2.0);
        m_icon.addPixmap(pixmap);
    }

    for (const QJsonValue &vv : jsonObject[QStringLiteral("aliases")].toArray())
        m_aliases << vv.toString();

    for (const QJsonValue &vv : jsonObject[QStringLiteral("versions")].toArray())
        m_versions << vv.toString();
    m_revision = jsonObject[QStringLiteral("revision")].toString();

    m_feedUrl = jsonObject[QStringLiteral("feed_url")].toString();
    const QJsonArray urlArray = jsonObject[QStringLiteral("urls")].toArray();
    for (const QJsonValue &url : urlArray)
        m_urls.append(url.toString());

    m_extra = jsonObject[QStringLiteral("extra")].toObject();
}

QString DocsetMetadata::sourceId() const
{
    return m_sourceId;
}

/*!
  Creates meta.json for specified docset \a version in the \a path.
*/
void DocsetMetadata::save(const QString &path, const QString &version)
{
    QScopedPointer<QFile> file(new QFile(path + QLatin1String("/meta.json")));
    if (!file->open(QIODevice::WriteOnly))
        return;

    QJsonObject jsonObject;

    jsonObject[QStringLiteral("sourceId")] = m_sourceId;
    jsonObject[QStringLiteral("name")] = m_name;
    jsonObject[QStringLiteral("title")] = m_title;

    if (!version.isEmpty())
        jsonObject[QStringLiteral("version")] = version;

    if (version == latestVersion() && !m_revision.isEmpty())
        jsonObject[QStringLiteral("revision")] = m_revision;

    if (!m_feedUrl.isEmpty())
        jsonObject[QStringLiteral("feed_url")] = m_feedUrl.toString();

    if (!m_urls.isEmpty()) {
        QJsonArray urls;
        for (const QUrl &url : m_urls)
            urls.append(url.toString());
        jsonObject[QStringLiteral("urls")] = urls;
    }

    if (!m_extra.isEmpty())
        jsonObject[QStringLiteral("extra")] = m_extra;

    file->write(QJsonDocument(jsonObject).toJson());
    file->close();

    file->setFileName(path + QLatin1String("/icon.png"));
    if (file->open(QIODevice::WriteOnly))
        file->write(m_rawIcon);
    file->close();

    file->setFileName(path + QLatin1String("/icon@2x.png"));
    if (file->open(QIODevice::WriteOnly))
        file->write(m_rawIcon2x);
    file->close();
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
    jsonObject[QStringLiteral("icon")] = QString::fromLocal8Bit(m_rawIcon.toBase64());
    jsonObject[QStringLiteral("icon2x")] = QString::fromLocal8Bit(m_rawIcon2x.toBase64());
    jsonObject[QStringLiteral("title")] = m_title;
    jsonObject[QStringLiteral("revision")] = m_revision;

    QJsonArray aliases;
    for (const QString &alias : m_aliases)
        aliases.append(alias);
    jsonObject[QStringLiteral("aliases")] = aliases;

    QJsonArray versions;
    for (const QString &version : m_versions)
        versions.append(version);
    jsonObject[QStringLiteral("versions")] = versions;

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

QIcon DocsetMetadata::icon() const
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

QStringList DocsetMetadata::versions() const
{
    return m_versions;
}

QString DocsetMetadata::latestVersion() const
{
    return m_versions.isEmpty() ? QString() : m_versions.first();
}

QString DocsetMetadata::revision() const
{
    return m_revision;
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
            metadata.m_versions << xml.text().toString();
        } else if (xml.name() == QLatin1String("url")) {
            if (xml.readNext() != QXmlStreamReader::Characters)
                continue;
            metadata.m_urls.append(xml.text().toString());
        }
    }

    return metadata;
}
