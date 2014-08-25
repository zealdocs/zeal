#include "mainwindow.h"
#include <QApplication>
#include <QLocalSocket>
#include <QProxyStyle>

#include <string>
#include <iostream>

using namespace std;

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QCommandLineParser>

// Sets up the command line parser.
void setupOptionParser(QCommandLineParser *parser) {
    parser->setApplicationDescription("zeal - Offline documentation browser.");
    parser->addHelpOption();
    parser->addVersionOption();
    QCommandLineOption queryOption(QStringList() << "q" << "query",
                                   "Query <search term>.", "term");
    parser->addOption(queryOption);
    QCommandLineOption forceRun(QStringList() << "f" << "force",
                                "Force the application run.");
    parser->addOption(forceRun);
}
#endif

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
    QApplication::setApplicationName("zeal");
    QApplication::setApplicationVersion(ZEAL_VERSION);
#ifdef WIN32
    a.setStyle(new ZealProxyStyle);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    QCommandLineParser optionParser;
    setupOptionParser(&optionParser);
    optionParser.process(a);
    // Extract the command line flags.
    QString queryParam = optionParser.value("query");
    bool runForce = optionParser.isSet("force");
#else
    QString queryParam;
    if (argc > 2 && QString(argv[1]) == "--query") {
        queryParam = argv[2];
    }
    bool runForce = false;
#endif

    // detect already running instance and optionally pass a search
    // query onto it.
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (!runForce && socket.waitForConnected(500)) {
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
