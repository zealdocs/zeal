#include "application.h"

#include "ui/mainwindow.h"

using namespace Zeal;
using namespace Zeal::Core;

Application::Application(const QString &query, QObject *parent) :
    QObject(parent),
    m_mainWindow(new MainWindow())
{
    if (!query.isEmpty())
        m_mainWindow->bringToFrontAndSearch(query);
    else if (!m_mainWindow->startHidden())
        m_mainWindow->show();
}

Application::~Application()
{

}

