/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_CORE_FILEMANAGER_H
#define ZEAL_CORE_FILEMANAGER_H

#include <QObject>

namespace Zeal {
namespace Core {

class FileManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FileManager)
public:
    explicit FileManager(QObject *parent = nullptr);

    bool removeRecursively(const QString &path);
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_FILEMANAGER_H
