/****************************************************************************
**
** Copyright (C) 2017 Oleg Shparber
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

#ifndef ZEAL_CORE_FILEMANAGER_H
#define ZEAL_CORE_FILEMANAGER_H

#include <QObject>

namespace Zeal {
namespace Core {

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);

    bool removeRecursively(const QString &path);

    static QString cacheLocation();
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_FILEMANAGER_H
