#include "mainwindow.h"
#include <QApplication>
#include <QLocalSocket>

#include <string>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    // detect already running instance
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        cerr << "Already running. Terminating." << endl;
        return -1; // Exit already a process running
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
