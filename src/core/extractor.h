#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QObject>

namespace Zeal {
namespace Core {

class Extractor : public QObject
{
    Q_OBJECT
public:
    explicit Extractor(QObject *parent = 0);

public slots:
    void extract(const QString &filePath, const QString &destination);

signals:
    void error(const QString &filePath, const QString &message);
    void completed(const QString &filePath);
};

} // namespace Core
} // namespace Zeal

#endif // EXTRACTOR_H
