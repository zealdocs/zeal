#include "core/application.h"
#include "registry/searchquery.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrlQuery>

#ifdef Q_OS_WIN32
#include <QProxyStyle>
#include <QStyleOption>
#endif

struct CommandLineParameters
{
    bool force;
    Zeal::SearchQuery query;
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
    parser.addPositionalArgument(QStringLiteral("url"), QObject::tr("dash[-plugin]:// URL"));
    parser.process(app);

    Zeal::SearchQuery query;

    /// TODO: Support dash-feed:// protocol
    const QString arg = parser.positionalArguments().value(0);
    if (arg.startsWith(QLatin1String("dash://"))) {
        query.setQuery(arg.mid(7));
    } else if (arg.startsWith(QLatin1String("dash-plugin://"))) {
        QUrlQuery urlQuery(arg.mid(14));

        const QString keys = urlQuery.queryItemValue(QStringLiteral("keys"));
        if (!keys.isEmpty())
            query.setKeywords(keys.split(QLatin1Char(',')));
        query.setQuery(urlQuery.queryItemValue(QStringLiteral("query")));
    } else {
        query.setQuery(parser.value(QStringLiteral("query")));
    }

    return CommandLineParameters{parser.isSet(QStringLiteral("force")), query};
}

/// TODO: Verify if this bug still exists in Qt 5.2+
#ifdef Q_OS_WIN32
class ZealProxyStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const
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
            QDataStream out(socket.data());
            out << clParams.query;
            socket->flush();
            return 0;
        }
    }

    QDir::setSearchPaths(QStringLiteral("docsetIcon"), {QStringLiteral(":/icons/docset")});
    QDir::setSearchPaths(QStringLiteral("typeIcon"), {QStringLiteral(":/icons/type")});

    QScopedPointer<Zeal::Core::Application> app(new Zeal::Core::Application(clParams.query));

    return qapp.exec();
}
