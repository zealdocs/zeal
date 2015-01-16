#include "core/application.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN32
#include <QProxyStyle>
#include <QStyleOption>
#endif

struct CommandLineParameters
{
    bool force;
    QString query;
};

CommandLineParameters parseCommandLine(const QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Zeal - Offline documentation browser."));
    parser.addHelpOption();
    parser.addVersionOption();

    /// TODO: [Qt 5.4] parser.addOption({{"f", "force"}, "Force the application run."});
    parser.addOption(QCommandLineOption({QStringLiteral("f"), QStringLiteral("force")},
                                        QObject::tr("Force the application run.")));
    parser.addOption(QCommandLineOption({QStringLiteral("q"), QStringLiteral("query")},
                                        QObject::tr("Query <search term>."),
                                        QStringLiteral("term")));
    parser.process(app);

    return {
        parser.isSet(QStringLiteral("force")),
        parser.value(QStringLiteral("query"))
    };
}

/// TODO: Verify if this bug still exists in Qt 5.2+
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
    QCoreApplication::setApplicationName(QStringLiteral("Zeal"));
    QCoreApplication::setApplicationVersion(ZEAL_VERSION);
    QCoreApplication::setOrganizationDomain(QStringLiteral("zealdocs.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("Zeal"));

    QApplication qapp(argc, argv);

#ifdef Q_OS_WIN32
    qapp.setStyle(new ZealProxyStyle());
#endif

    const CommandLineParameters clParams = parseCommandLine(qapp);

    // Detect already running instance and optionally pass a search query to it.
    if (!clParams.force) {
        QScopedPointer<QLocalSocket> socket(new QLocalSocket());
        socket->connectToServer(Zeal::Core::Application::localServerName());

        if (socket->waitForConnected(500)) {
            if (!clParams.query.isEmpty()) {
                socket->write(clParams.query.toLocal8Bit());
                socket->flush();
            } else {
                QTextStream(stdout) << QObject::tr("Already running. Terminating.") << endl;
            }

            return 0;
        }
    }

    // look for icons in:
    // 1. user's data directory (same as docsets dir, but in icons/)
    // 2. executable directory/icons
    // 3. on unix, standard/legacy install location
    // 4. current directory/icons
    QStringList searchPaths;
    searchPaths << QStandardPaths::locateAll(QStandardPaths::DataLocation, QStringLiteral("icons"),
                                             QStandardPaths::LocateDirectory);
    searchPaths << QDir(QCoreApplication::applicationDirPath())
                   .absoluteFilePath(QStringLiteral("icons"));
#ifndef Q_OS_WIN32
    searchPaths << QStringLiteral("/usr/share/pixmaps/zeal");
#endif
    searchPaths << QStringLiteral("./icons");
    QDir::setSearchPaths(QStringLiteral("icons"), searchPaths);

    QScopedPointer<Zeal::Core::Application> app(new Zeal::Core::Application(clParams.query));

    return qapp.exec();
}
