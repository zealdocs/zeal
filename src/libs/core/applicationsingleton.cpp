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

#include "applicationsingleton.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QScopedPointer>
#include <QSharedMemory>

using namespace Zeal::Core;

static Q_LOGGING_CATEGORY(log, "zeal.core.applicationsingleton")

struct SharedData {
    qint64 primaryPid;
};

ApplicationSingleton::ApplicationSingleton(QObject *parent)
    : QObject(parent)
{
    if (QCoreApplication::instance() == nullptr) {
        qFatal("QCoreApplication (or derived type) must be created before ApplicationSingleton.");
    }

    m_id = computeId();
    qCDebug(log, "Singleton ID: %s", qPrintable(m_id));

    m_sharedMemory = new QSharedMemory(m_id, this);

    m_isPrimary = m_sharedMemory->create(sizeof(SharedData));
    if (m_isPrimary) {
        setupPrimary();
        return;
    }

#ifdef Q_OS_UNIX
    // Verify it's not a segment that survived an application crash.
    m_sharedMemory->attach();
    m_sharedMemory->detach();

    m_isPrimary = m_sharedMemory->create(sizeof(SharedData));
    if (m_isPrimary) {
        setupPrimary();
        return;
    }
#endif

    if (!m_sharedMemory->attach(QSharedMemory::ReadOnly)) {
        qCWarning(log) << "Cannot attach to the shared memory segment:"
                       << m_sharedMemory->errorString();
        return;
    }

    setupSecondary();
}

bool ApplicationSingleton::isPrimary() const
{
    return m_isPrimary;
}

bool ApplicationSingleton::isSecondary() const
{
    return !m_isPrimary;
}

qint64 ApplicationSingleton::primaryPid() const
{
    return m_primaryPid;
}

bool ApplicationSingleton::sendMessage(QByteArray &data, int timeout)
{
    // No support for primary to secondary communication.
    if (m_isPrimary) {
        return false;
    }

    QScopedPointer<QLocalSocket, QScopedPointerDeleteLater> socket(new QLocalSocket);
    socket->connectToServer(m_id);
    if (!socket->waitForConnected(timeout)) {
        qCWarning(log) << "Cannot connect to the local service:"
                       << socket->errorString();
        return false;
    }

    socket->write(data);
    socket->flush(); // Required for Linux.
    return socket->waitForBytesWritten(timeout);
}

void ApplicationSingleton::setupPrimary()
{
    m_primaryPid = QCoreApplication::applicationPid();

    qCInfo(log, "Starting as a primary instance. (PID: %lld)", m_primaryPid);

    m_sharedMemory->lock();
    SharedData *sd = static_cast<SharedData *>(m_sharedMemory->data());
    sd->primaryPid = m_primaryPid;
    m_sharedMemory->unlock();

    QLocalServer::removeServer(m_id);

    m_localServer = new QLocalServer(this);
    m_localServer->setSocketOptions(QLocalServer::UserAccessOption);

    connect(m_localServer, &QLocalServer::newConnection, [this] {
        QLocalSocket *socket = m_localServer->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, [this, socket] {
            QByteArray data = socket->readAll();
            emit messageReceived(data);
            socket->deleteLater();
        });
    });

    if (!m_localServer->listen(m_id)) {
        qCWarning(log) << "Cannot start the local service:"
                       << m_localServer->errorString();
        return;
    }
}

void ApplicationSingleton::setupSecondary()
{
    m_sharedMemory->lock();
    SharedData *sd = static_cast<SharedData *>(m_sharedMemory->data());
    m_primaryPid = sd->primaryPid;
    m_sharedMemory->unlock();

    qCInfo(log, "Starting as a secondary instance. (Primary PID: %lld)", m_primaryPid);
}

QString ApplicationSingleton::computeId()
{
    // Make sure the result can be used as a name for the local socket.
    static const QByteArray::Base64Options base64Options
            = QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals;

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(QCoreApplication::applicationName().toUtf8());
    hash.addData(QCoreApplication::organizationName().toUtf8());
    hash.addData(QCoreApplication::organizationDomain().toUtf8());
    // Support multi-user setup.
    hash.addData(QDir::homePath().toUtf8());

    return QString::fromLatin1(hash.result().toBase64(base64Options));
}
