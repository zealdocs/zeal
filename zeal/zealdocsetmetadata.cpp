#include "zealdocsetmetadata.h"

ZealDocsetMetadata::ZealDocsetMetadata():
    valid(true)
{
}

void ZealDocsetMetadata::read(const QJsonObject &json){
    version = json["version"].toString();
    feedUrl = json["feed_url"].toString();
    QJsonArray urlArray = json["urls"].toArray();
    foreach(auto url, urlArray){
        urls.append( url.toString() );
    }
}

void ZealDocsetMetadata::read(QXmlStreamReader &xml){
    while(!xml.atEnd()){
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartDocument){
            continue;
        }
        // Try to pull out the relevant data
        if(token == QXmlStreamReader::StartElement){
            if(xml.name() == "version"){
                if(xml.readNext()!=QXmlStreamReader::Characters) continue;
                version = xml.text().toString();
            } else if(xml.name() == "url"){
                if(xml.readNext()!=QXmlStreamReader::Characters) continue;
                urls.append( xml.text().toString() );
            }
        }
    }
}

void ZealDocsetMetadata::read(const QString &path) {
    QFile file(path);
    if(file.open(QIODevice::ReadOnly)){
        QByteArray data = file.readAll();
        QJsonDocument doc( QJsonDocument::fromJson(data) );
        read( doc.object() );
        file.close();
    } else {
        valid = false;
    }
}

void ZealDocsetMetadata::write(const QString &path) const {
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)){
        QJsonObject meta;
        write(meta);
        QJsonDocument doc(meta);
        file.write(doc.toJson());
        file.close();
    }
}

void ZealDocsetMetadata::write(QJsonObject &json) const {
    json["version"] = version;
    QJsonArray urlArray;
    foreach(auto url , urls){
        urlArray.append(url);
    }

    json["urls"] = urlArray;
    json["feed_url"] = feedUrl;
}

bool ZealDocsetMetadata::isValid() const {
    return valid;
}

QStringList ZealDocsetMetadata::getUrls() const {
    return urls;
}

QString ZealDocsetMetadata::getPrimaryUrl() const {
    // Find preferred mirror from feed url host
    QUrl feedUrl(getFeedURL());

    QRegExp regex("[^:]+://"+feedUrl.host().replace(".", "\\.")+".*");

    int idx = urls.indexOf( regex );

    if(idx<0){
        idx = qrand() % urls.count();
    }

    return urls[idx];
}

void ZealDocsetMetadata::addUrl(const QString &url){
    urls.append(url);
}

int ZealDocsetMetadata::getNumUrls() const {
    return urls.length();
}

void ZealDocsetMetadata::setVersion(const QString &ver){
    version = ver;
}

QString ZealDocsetMetadata::getVersion() const {
    return version;
}

QString ZealDocsetMetadata::getFeedURL() const {
    return feedUrl;
}

void ZealDocsetMetadata::setFeedURL(const QString &url){
    feedUrl = url;
}
