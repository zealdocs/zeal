/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <core/application.h>
#include <core/applicationsingleton.h>
#include <registry/searchquery.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>

#ifdef Q_OS_WIN32
#include <QSettings>

#include <Windows.h>
#endif

#include <cstdlib>

using namespace Zeal;

struct CommandLineParameters
{
    bool force;
    bool preventActivation;
    Registry::SearchQuery query;
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

    parser.addOption({{QStringLiteral("f"), QStringLiteral("force")},
                      QObject::tr("Force the application run.")});

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
    clParams.preventActivation = false;

#ifdef Q_OS_WIN32
    clParams.registerProtocolHandlers = parser.isSet(QStringLiteral("register"));
    clParams.unregisterProtocolHandlers = parser.isSet(QStringLiteral("unregister"));

    if (clParams.registerProtocolHandlers && clParams.unregisterProtocolHandlers) {
        QTextStream(stderr) << QObject::tr("Parameter conflict: --register and --unregister.\n");
        ::exit(EXIT_FAILURE);
    }
#endif

    // TODO: Support dash-feed:// protocol
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

        const QString preventActivation
                = urlQuery.queryItemValue(QStringLiteral("prevent_activation"));
        clParams.preventActivation = preventActivation == QLatin1String("true");
    } else {
        clParams.query.setQuery(arg);
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
    reg->setValue(QStringLiteral("Default"), QVariant(appPath + QLatin1String(" %1")));
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
#endif

int main(int argc, char *argv[])
{
    // Do not allow Qt version lower than the app was compiled with.
    QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR);

    QCoreApplication::setApplicationName(QStringLiteral("Zeal"));
    QCoreApplication::setApplicationVersion(ZEAL_VERSION);
    QCoreApplication::setOrganizationDomain(QStringLiteral("zealdocs.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("Zeal"));

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QScopedPointer<QApplication> qapp(new QApplication(argc, argv));

    const CommandLineParameters clParams = parseCommandLine(qapp->arguments());

#ifdef Q_OS_WIN32
    const static QHash<QString, QString> protocols = {
        {QStringLiteral("dash"), QStringLiteral("Dash Protocol")},
        {QStringLiteral("dash-plugin"), QStringLiteral("Dash Plugin Protocol")}
    };

    if (clParams.unregisterProtocolHandlers) {
        unregisterProtocolHandlers(protocols);
        return EXIT_SUCCESS;
    } else {
        registerProtocolHandlers(protocols, clParams.registerProtocolHandlers);
        if (clParams.registerProtocolHandlers)
            return EXIT_SUCCESS;
    }
#endif

    QScopedPointer<Core::ApplicationSingleton> appSingleton(new Core::ApplicationSingleton());
    if (appSingleton->isSecondary()) {
#ifdef Q_OS_WIN32
        ::AllowSetForegroundWindow(appSingleton->primaryPid());
#endif
        QByteArray ba;
        QDataStream out(&ba, QIODevice::WriteOnly);
        out << clParams.query << clParams.preventActivation;
        // TODO: Check for a possible error.
        appSingleton->sendMessage(ba);
        return EXIT_SUCCESS;
    }

    // Set application-wide window icon. All message boxes and other windows will use it by default.
    qapp->setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"),
                                         QIcon(QStringLiteral(":/zeal.ico"))));

    QDir::setSearchPaths(QStringLiteral("typeIcon"), {QStringLiteral(":/icons/type")});

    QScopedPointer<Core::Application> app(new Core::Application());

    QObject::connect(appSingleton.data(), &Core::ApplicationSingleton::messageReceived,
                     [&app](const QByteArray &data) {
        Registry::SearchQuery query;
        bool preventActivation;

        QDataStream in(data);
        in >> query >> preventActivation;

        app->executeQuery(query, preventActivation);
    });

    if (!clParams.query.isEmpty()) {
        QTimer::singleShot(0, [&app, clParams] {
            app->executeQuery(clParams.query, clParams.preventActivation);
        });
    }

    return qapp->exec();
}
