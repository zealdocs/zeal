#include "mainwindow.h"
#include <QApplication>
#include <QLocalSocket>
#include <QProxyStyle>

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

#ifdef WIN32
class ZealProxyStyle : public QProxyStyle {
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter* painter, const QWidget *widget = 0) const {
        if (element == PE_FrameLineEdit && option->styleObject) {
            option->styleObject->setProperty("_q_no_animation", true);
            // Workaround for a probable bug in QWindowsVistaStyle - for example opening the 'String (CommandEvent)'
            // item from wxPython docset (available at http://wxpython.org/Phoenix/docsets) causes very high CPU usage.
            // Some rough debugging shows that the 'd->startAnimation(t);' call in QWindowsVistaStyle::drawPrimitive
            // is the cuplrit and setting _q_no_animation to true here fixes the issue.
        }
        return QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef WIN32
    a.setStyle(new ZealProxyStyle);
#endif
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

    if (!w.startHidden())
        w.show();

    if(!queryParam.isEmpty()) {
        w.bringToFrontAndSearch(queryParam);
    }
    return a.exec();
}
