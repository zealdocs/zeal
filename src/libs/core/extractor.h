// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_EXTRACTOR_H
#define ZEAL_CORE_EXTRACTOR_H

#include <QObject>

struct archive;

namespace Zeal::Core {

class Extractor final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Extractor)
public:
    explicit Extractor(QObject *parent = nullptr);
    ~Extractor() override = default;

    void extract(const QString &sourceFile, const QString &destination, const QString &root = QString());
    void installTarixDocset(const QString &sourceFile,
                            const QString &indexArchiveFile,
                            const QString &destination,
                            const QString &root);

signals:
    void error(const QString &filePath, const QString &message);
    void completed(const QString &filePath);
    void progress(const QString &filePath, qint64 extracted, qint64 total);

private:
    struct ExtractInfo
    {
        archive *archiveHandle;
        QString filePath;
        qint64 totalBytes;
        qint64 extractedBytes;
    };

    bool extractEntries(const QString &sourceFile,
                        const QString &destination,
                        const QString &root,
                        const QString &skipPrefix);
    static bool extractTarixIndex(const QString &indexArchiveFile, const QString &indexPath);

    void emitProgress(ExtractInfo &info);
};

} // namespace Zeal::Core

#endif // ZEAL_CORE_EXTRACTOR_H
