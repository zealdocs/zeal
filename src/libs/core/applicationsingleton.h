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

#ifndef ZEAL_CORE_APPLICATIONSINGLETON_H
#define ZEAL_CORE_APPLICATIONSINGLETON_H

#include <QObject>

class QLocalServer;
class QSharedMemory;

namespace Zeal {
namespace Core {

class ApplicationSingleton final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ApplicationSingleton)
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
