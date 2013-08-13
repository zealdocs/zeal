#include "mainwindow.h"
#include <QApplication>
#include <QLocalSocket>

#include <string>
#include <iostream>

using namespace std;

QString getQueryParam(QStringList arguments)
{
    // Poor mans arg parser
    for (int i = 1; i < arguments.size(); ++i) {
        if(arguments.at(i) == "--query") {
            if(arguments.size() > i + 1) {
                return arguments.at(i + 1);
            } else {
                cerr << "Usage: " << arguments.at(0).toStdString() << " --query <search term>";
                exit(1);
            }
        }
    }

    return "";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString queryParam = getQueryParam(a.arguments());

    // detect already running instance and optionally pass a search
    // query onto it.
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        if(!queryParam.isEmpty()) {
            QByteArray msg;
            msg.append(queryParam);
            socket.write(msg);
            socket.flush();
            socket.close();
        } else {
            cerr << "Already running. Terminating." << endl;
        }
        return -1; // Exit already a process running
    }

    MainWindow w;
    w.show();
    if(!queryParam.isEmpty()) {
        w.bringToFrontAndSearch(queryParam);
    }
    return a.exec();
}
