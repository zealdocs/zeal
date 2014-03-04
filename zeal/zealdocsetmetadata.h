#ifndef ZEALDOCSETMETADATA_H
#define ZEALDOCSETMETADATA_H
#include <QDebug>
#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QFile>


class ZealDocsetMetadata //: public QObject
{
    //Q_OBJECT
public:
    ZealDocsetMetadata();
    void read(const QJsonObject &json);
    void read(QXmlStreamReader &xml);
    void read(const QString &path);
    void write(const QString &path) const;
    void write(QJsonObject &json) const;
    QStringList getUrls() const;
    int getNumUrls() const;
    QString getVersion() const;
    QString getFeedURL() const;
    void setFeedURL(const QString &url);
private:
    QStringList urls;
    QString version;
    QString feedUrl;
};

Q_DECLARE_METATYPE(ZealDocsetMetadata);

#endif // ZEALDOCSETMETADATA_H
