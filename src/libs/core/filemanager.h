// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_FILEMANAGER_H
#define ZEAL_CORE_FILEMANAGER_H

#include <QObject>

namespace Zeal::Core {

class FileManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FileManager)
public:
    explicit FileManager(QObject *parent = nullptr);
    ~FileManager() override = default;

    bool removeRecursively(const QString &path);
};

} // namespace Zeal::Core

#endif // ZEAL_CORE_FILEMANAGER_H
