#include "application.h"

#include "ui/mainwindow.h"

#include <QLocalServer>
#include <QLocalSocket>

using namespace Zeal;
using namespace Zeal::Core;

namespace {
const char *LocalServerName = "ZealLocalServer";
}

Application::Application(const QString &query, QObject *parent) :
    QObject(parent),
    m_localServer(new QLocalServer(this)),
    m_mainWindow(new MainWindow())
{
    // Server for detecting already running instances
    connect(m_localServer, &QLocalServer::newConnection, [this]() {
        QLocalSocket *connection = m_localServer->nextPendingConnection();
        // Wait a little while the other side writes the bytes
        connection->waitForReadyRead();
        if (connection->bytesAvailable())
            m_mainWindow->bringToFrontAndSearch(QString::fromLocal8Bit(connection->readAll()));
    });
    /// TODO: Verify if removeServer() is needed
    QLocalServer::removeServer(LocalServerName);  // remove in case previous instance crashed
    m_localServer->listen(LocalServerName);

    if (!query.isEmpty())
        m_mainWindow->bringToFrontAndSearch(query);
    else if (!m_mainWindow->startHidden())
        m_mainWindow->show();
}

Application::~Application()
{
}

QString Application::localServerName()
{
    return LocalServerName;
}

