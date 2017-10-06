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

#ifndef ZEAL_CORE_APPLICATIONSINGLETON_H
#define ZEAL_CORE_APPLICATIONSINGLETON_H

#include <QObject>

class QLocalServer;
class QSharedMemory;

namespace Zeal {
namespace Core {

class ApplicationSingleton : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationSingleton(QObject *parent = nullptr);

    bool isPrimary() const;
    bool isSecondary() const;
    qint64 primaryPid() const;

    bool sendMessage(QByteArray &data, int timeout = 500);

signals:
    void messageReceived(const QByteArray &data);

private:
    void setupPrimary();
    void setupSecondary();

    static QString computeId();

    QString m_id;

    bool m_isPrimary = false;
    qint64 m_primaryPid = 0;

    QSharedMemory *m_sharedMemory = nullptr;
    QLocalServer *m_localServer = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_APPLICATIONSINGLETON_H
