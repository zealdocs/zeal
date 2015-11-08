/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "core/application.h"
#include "registry/searchquery.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QLocalSocket>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrlQuery>

#ifdef Q_OS_WIN32
#include <QProxyStyle>
#include <QSettings>
#include <QStyleOption>
#endif

struct CommandLineParameters
{
    bool force;
    Zeal::SearchQuery query;
#ifdef Q_OS_WIN32
    bool registerProtocolHandlers;
    bool unregisterProtocolHandlers;
#endif
};

QString stripParameterUrl(const QString &url, const QString &scheme)
{
    QStringRef ref = url.midRef(scheme.length() + 1);
    if (ref.startsWith(QLatin1String("//")))
        ref = ref.mid(2);
    if (ref.endsWith(QLatin1Char('/')))
        ref = ref.left(ref.length() - 1);
    return ref.toString();
}

CommandLineParameters parseCommandLine(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Zeal - Offline documentation browser."));
    parser.addHelpOption();
    parser.addVersionOption();

    /// TODO: [Qt 5.4] parser.addOption({{"f", "force"}, "Force the application run."});
    parser.addOption(QCommandLineOption({QStringLiteral("f"), QStringLiteral("force")},
                                        QObject::tr("Force the application run.")));
    /// TODO: [0.3.0] Remove --query support
    parser.addOption(QCommandLineOption({QStringLiteral("q"), QStringLiteral("query")},
                                        QObject::tr("[DEPRECATED] Query <search term>."),
                                        QStringLiteral("term")));
#ifdef Q_OS_WIN32
    parser.addOption(QCommandLineOption({QStringLiteral("register")},
                                        QObject::tr("Register protocol handlers")));
    parser.addOption(QCommandLineOption({QStringLiteral("unregister")},
                                        QObject::tr("Unregister protocol handlers")));
#endif
    parser.addPositionalArgument(QStringLiteral("url"), QObject::tr("dash[-plugin]:// URL"));
    parser.process(arguments);

    CommandLineParameters clParams;
    clParams.force = parser.isSet(QStringLiteral("force"));

#ifdef Q_OS_WIN32
    clParams.registerProtocolHandlers = parser.isSet(QStringLiteral("register"));
    clParams.unregisterProtocolHandlers = parser.isSet(QStringLiteral("unregister"));

    if (clParams.registerProtocolHandlers && clParams.unregisterProtocolHandlers) {
        QTextStream(stderr) << QObject::tr("Parameter conflict: --register and --unregister.\n");
        ::exit(EXIT_FAILURE);
    }
#endif

    if (parser.isSet(QStringLiteral("query"))) {
        clParams.query.setQuery(parser.value(QStringLiteral("query")));
    } else {
        /// TODO: Support dash-feed:// protocol
        const QString arg
                = QUrl::fromPercentEncoding(parser.positionalArguments().value(0).toUtf8());
        if (arg.startsWith(QLatin1String("dash:"))) {
            clParams.query.setQuery(stripParameterUrl(arg, QStringLiteral("dash")));
        } else if (arg.startsWith(QLatin1String("dash-plugin:"))) {
            const QUrlQuery urlQuery(stripParameterUrl(arg, QStringLiteral("dash-plugin")));
            const QString keys = urlQuery.queryItemValue(QStringLiteral("keys"));
            if (!keys.isEmpty())
                clParams.query.setKeywords(keys.split(QLatin1Char(',')));
            clParams.query.setQuery(urlQuery.queryItemValue(QStringLiteral("query")));
        } else {
            clParams.query.setQuery(arg);
        }
    }

    return clParams;
}

#ifdef Q_OS_WIN32
void registerProtocolHandler(const QString &scheme, const QString &description)
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\") + scheme;

    QScopedPointer<QSettings> reg(new QSettings(regPath, QSettings::NativeFormat));

    reg->setValue(QStringLiteral("Default"), description);
    reg->setValue(QStringLiteral("URL Protocol"), QString());

    reg->beginGroup(QStringLiteral("DefaultIcon"));
    reg->setValue(QStringLiteral("Default"), QString("%1,1").arg(appPath));
    reg->endGroup();

    reg->beginGroup(QStringLiteral("shell"));
    reg->beginGroup(QStringLiteral("open"));
    reg->beginGroup(QStringLiteral("command"));
    reg->setValue(QStringLiteral("Default"), appPath + QLatin1String(" %1"));
}

void registerProtocolHandlers(const QHash<QString, QString> &protocols, bool force = false)
{
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
    QScopedPointer<QSettings> reg(new QSettings(regPath, QSettings::NativeFormat));

    const QStringList groups = reg->childGroups();
    for (auto it = protocols.cbegin(); it != protocols.cend(); ++it) {
        if (force || !groups.contains(it.key()))
            registerProtocolHandler(it.key(), it.value());
    }
}

void unregisterProtocolHandlers(const QHash<QString, QString> &protocols)
{
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
    QScopedPointer<QSettings> reg(new QSettings(regPath, QSettings::NativeFormat));

    for (auto it = protocols.cbegin(); it != protocols.cend(); ++it)
        reg->remove(it.key());
}

/// TODO: Verify if this bug still exists in Qt 5.2+
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

    QScopedPointer<QApplication> qapp(new QApplication(argc, argv));

    const CommandLineParameters clParams = parseCommandLine(qapp->arguments());

#ifdef Q_OS_WIN32
    const static QHash<QString, QString> protocols = {
        {QStringLiteral("dash"), QStringLiteral("Dash Protocol")},
        {QStringLiteral("dash-plugin"), QStringLiteral("Dash Plugin Protocol")}
    };

    if (clParams.unregisterProtocolHandlers) {
        unregisterProtocolHandlers(protocols);
        ::exit(EXIT_SUCCESS);
    } else {
        registerProtocolHandlers(protocols, clParams.registerProtocolHandlers);
        if (clParams.registerProtocolHandlers)
            ::exit(EXIT_SUCCESS);
    }

    qapp->setStyle(new ZealProxyStyle());
#endif


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

    // Check for SQLite plugin
    /// TODO: Specific to docset format and should be handled accordingly in the future
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        const int ret = QMessageBox::critical(nullptr, QStringLiteral("Zeal"),
                                              QObject::tr("Qt SQLite driver is not available."),
                                              QMessageBox::Close, QMessageBox::Help);
        if (ret == QMessageBox::Help)
            QDesktopServices::openUrl(QStringLiteral("https://zealdocs.org/contact.html"));
        return 0;
    }

    QDir::setSearchPaths(QStringLiteral("typeIcon"), {QStringLiteral(":/icons/type")});

    QScopedPointer<Zeal::Core::Application> app(new Zeal::Core::Application(clParams.query));

    return qapp->exec();
}
