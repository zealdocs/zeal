#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QObject>

struct archive;

namespace Zeal {
namespace Core {

class Extractor : public QObject
{
    Q_OBJECT
public:
    explicit Extractor(QObject *parent = nullptr);

public slots:
    void extract(const QString &filePath, const QString &destination, const QString &root = QString());

signals:
    void error(const QString &filePath, const QString &message);
    void completed(const QString &filePath);
    void progress(const QString &filePath, qint64 extracted, qint64 total);

private:
    struct ExtractInfo {
        Extractor *extractor;
        archive *archiveHandle;
        QString filePath;
        qint64 totalBytes;
        qint64 extractedBytes;
    };

    static void progressCallback(void *ptr);
};

} // namespace Core
} // namespace Zeal

#endif // EXTRACTOR_H
