// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_APPLICATIONSINGLETON_H
#define ZEAL_CORE_APPLICATIONSINGLETON_H

#include <QObject>

class QLocalServer;
class QSharedMemory;

namespace Zeal::Core {

class ApplicationSingleton final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ApplicationSingleton)
public:
    enum class State {
        Primary,
        Secondary,
        Failed,
    };

    explicit ApplicationSingleton(QObject *parent = nullptr);
    ~ApplicationSingleton() override = default;

    State state() const;
    qint64 primaryPid() const;

    bool sendMessage(QByteArray &data, int timeout = 500);

signals:
    void messageReceived(const QByteArray &data);

private:
    void setupPrimary();
    void setupSecondary();

    static QString computeId();

    QString m_id;

    State m_state = State::Failed;
    qint64 m_primaryPid = 0;

    QSharedMemory *m_sharedMemory = nullptr;
    QLocalServer *m_localServer = nullptr;
};

} // namespace Zeal::Core

#endif // ZEAL_CORE_APPLICATIONSINGLETON_H
