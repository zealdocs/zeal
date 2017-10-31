/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_CORE_EXTRACTOR_H
#define ZEAL_CORE_EXTRACTOR_H

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

#endif // ZEAL_CORE_EXTRACTOR_H
