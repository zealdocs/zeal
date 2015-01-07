#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocalSocket>
#include <QProxyStyle>
#include <QStandardPaths>
#include <QTextStream>

// Sets up the command line parser.
void setupOptionParser(QCommandLineParser *parser)
{
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

#ifdef Q_OS_WIN32
class ZealProxyStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = 0) const
    {
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

    QCoreApplication::setApplicationName(QStringLiteral("Zeal"));
    QCoreApplication::setApplicationVersion(ZEAL_VERSION);
    QCoreApplication::setOrganizationDomain(QStringLiteral("zealdocs.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("Zeal"));

#ifdef Q_OS_WIN32
    a.setStyle(new ZealProxyStyle);
#endif

    QCommandLineParser optionParser;
    setupOptionParser(&optionParser);
    optionParser.process(a);
    // Extract the command line flags.
    QString queryParam = optionParser.value("query");
    bool runForce = optionParser.isSet("force");

    // detect already running instance and optionally pass a search
    // query onto it.
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (!runForce && socket.waitForConnected(500)) {
        if (!queryParam.isEmpty()) {
            QByteArray msg;
            msg.append(queryParam);
            socket.write(msg);
            socket.flush();
            socket.close();
        } else {
            QTextStream(stderr) << "Already running. Terminating." << endl;
        }
        return -1; // Exit already a process running
    }

    // look for icons in:
    // 1. user's data directory (same as docsets dir, but in icons/)
    // 2. executable directory/icons
    // 3. on unix, standard/legacy install location
    // 4. current directory/icons
    QDir::setSearchPaths("icons",
                         QStandardPaths::locateAll(QStandardPaths::DataLocation, "icons",
                                                   QStandardPaths::LocateDirectory));
    QDir::addSearchPath("icons",
                        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("icons"));
#ifndef Q_OS_WIN32
    QDir::addSearchPath("icons", "/usr/share/pixmaps/zeal");
#endif
    QDir::addSearchPath("icons", "./icons");

    MainWindow w;

    if (!w.startHidden())
        w.show();

    if (!queryParam.isEmpty())
        w.bringToFrontAndSearch(queryParam);

    return a.exec();
}
