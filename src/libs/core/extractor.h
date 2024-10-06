// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_EXTRACTOR_H
#define ZEAL_CORE_EXTRACTOR_H

#include <QObject>

struct archive;

namespace Zeal {
namespace Core {

class Extractor final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Extractor)
public:
    explicit Extractor(QObject *parent = nullptr);

public slots:
    void extract(const QString &sourceFile,
                 const QString &destination,
                 const QString &root = QString());

signals:
    void error(const QString &filePath, const QString &message);
    void completed(const QString &filePath);
    void progress(const QString &filePath, qint64 extracted, qint64 total);

private:
    struct ExtractInfo {
        archive *archiveHandle;
        QString filePath;
        qint64 totalBytes;
        qint64 extractedBytes;
    };

    void emitProgress(ExtractInfo &info);
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_EXTRACTOR_H
