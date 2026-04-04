// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

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

#ifdef Q_OS_WINDOWS
#include <QAbstractNativeEventFilter>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

#include <Windows.h>

#include <utility> // for std::ignore
#endif

#include <cstdlib>

namespace {

#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
// Windows fills new window client areas with COLOR_WINDOW (always white, even in dark mode)
// via WM_ERASEBKGND before Qt gets to paint. This filter intercepts the message and fills
// with the current palette color instead, preventing a white flash on window creation.
// See QTBUG-106583 and https://codereview.qt-project.org/c/qt/qtbase/+/440060 (reverted).
class DarkModeEraseFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        Q_UNUSED(eventType)
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_ERASEBKGND) {
            const QColor bg = QApplication::palette().color(QPalette::Window);
            auto *hdc = reinterpret_cast<HDC>(msg->wParam);
            RECT rc;
            GetClientRect(msg->hwnd, &rc);
            HBRUSH brush = CreateSolidBrush(RGB(bg.red(), bg.green(), bg.blue()));
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
            *result = 1;

            // Remove ourselves after the first paint — only needed for startup.
            QApplication::instance()->removeNativeEventFilter(this);

            return true;
        }

        return false;
    }
};
#endif

struct CommandLineParameters
{
    bool forceMinimized;
    bool preventActivation;

#ifdef Q_OS_WINDOWS
    bool attachConsole;
    bool registerProtocolHandlers;
    bool unregisterProtocolHandlers;
#endif

    Zeal::Registry::SearchQuery query;
};

QString stripParameterUrl(const QString &url, const QString &scheme)
{
    QString str = url.mid(scheme.length() + 1);

    if (str.startsWith(QLatin1String("//"))) {
        str = str.mid(2);
    }

    if (str.endsWith(QLatin1Char('/'))) {
        str = str.left(str.length() - 1);
    }

    return str;
}

CommandLineParameters parseCommandLine(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Zeal - Offline documentation browser."));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({{QStringLiteral("minimized"), QObject::tr("Start minimized regardless of settings.")}});

#ifdef Q_OS_WINDOWS
    parser.addOptions({{QStringLiteral("attach-console"), QObject::tr("Attach console for logging.")},
                       {QStringLiteral("register"), QObject::tr("Register protocol handlers.")},
                       {QStringLiteral("unregister"), QObject::tr("Unregister protocol handlers.")}});
#endif

    parser.addPositionalArgument(QStringLiteral("url"), QObject::tr("dash[-plugin]:// URL"));
    parser.process(arguments);

    CommandLineParameters clParams;
    clParams.forceMinimized = parser.isSet(QStringLiteral("minimized"));
    clParams.preventActivation = false;

#ifdef Q_OS_WINDOWS
    clParams.attachConsole = parser.isSet(QStringLiteral("attach-console"));
    clParams.registerProtocolHandlers = parser.isSet(QStringLiteral("register"));
    clParams.unregisterProtocolHandlers = parser.isSet(QStringLiteral("unregister"));

    if (clParams.registerProtocolHandlers && clParams.unregisterProtocolHandlers) {
        QTextStream(stderr) << QObject::tr("Parameter conflict: --register and --unregister.\n");
        ::exit(EXIT_FAILURE);
    }
#endif

    // TODO: Support dash-feed:// protocol
    const QString arg = QUrl::fromPercentEncoding(parser.positionalArguments().value(0).toUtf8());

    if (arg.startsWith(QLatin1String("dash:"))) {
        clParams.query.setQuery(stripParameterUrl(arg, QStringLiteral("dash")));
    } else if (arg.startsWith(QLatin1String("dash-plugin:"))) {
        const QUrlQuery urlQuery(stripParameterUrl(arg, QStringLiteral("dash-plugin")));

        const QString keys = urlQuery.queryItemValue(QStringLiteral("keys"));
        if (!keys.isEmpty()) {
            clParams.query.setKeywords(keys.split(QLatin1Char(',')));
        }

        clParams.query.setQuery(urlQuery.queryItemValue(QStringLiteral("query")));

        const QString preventActivation = urlQuery.queryItemValue(QStringLiteral("prevent_activation"));
        clParams.preventActivation = preventActivation == QLatin1String("true");
    } else {
        clParams.query.setQuery(arg);
    }

    return clParams;
}

#ifdef Q_OS_WINDOWS
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
        if (force || !groups.contains(it.key())) {
            registerProtocolHandler(it.key(), it.value());
        }
    }
}

void unregisterProtocolHandlers(const QHash<QString, QString> &protocols)
{
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
    QScopedPointer<QSettings> reg(new QSettings(regPath, QSettings::NativeFormat));

    for (auto it = protocols.cbegin(); it != protocols.cend(); ++it) {
        reg->remove(it.key());
    }
}
#endif

} // namespace

int main(int argc, char *argv[])
{
    // Do not allow Qt version lower than the app was compiled with.
    QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR)

    QCoreApplication::setApplicationName(QStringLiteral("Zeal"));
    QCoreApplication::setApplicationVersion(ZEAL_VERSION);
    QCoreApplication::setOrganizationDomain(QStringLiteral("zealdocs.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("Zeal"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Handle --version before creating QApplication to avoid
    // initializing the platform/graphics stack just to print a version string.
    {
        QCoreApplication coreApp(argc, argv);
        QCommandLineParser parser;
        parser.addVersionOption();
        parser.parse(coreApp.arguments());
        if (parser.isSet(QStringLiteral("version"))) {
            parser.showVersion();
        }
    }

// Use Fusion style on Windows 10 & 11. This enables proper dark mode support.
// See https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5.
// TODO: Make style configurable, detect -style argument.
#if defined(Q_OS_WINDOWS) && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
    const auto osName = QSysInfo::prettyProductName();
    if (osName.startsWith("Windows 10") || osName.startsWith("Windows 11")) {
        QApplication::setStyle("fusion");
    }
#endif

    QScopedPointer<QApplication> qapp(new QApplication(argc, argv));

#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    DarkModeEraseFilter darkModeEraseFilter;
    if (qapp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        qapp->installNativeEventFilter(&darkModeEraseFilter);
    }
#endif

    const CommandLineParameters clParams = parseCommandLine(qapp->arguments());

#ifdef Q_OS_WINDOWS
    const static QHash<QString, QString> protocols = {{QStringLiteral("dash"),
                                                       QStringLiteral("URL:Dash Protocol (Zeal)")},
                                                      {QStringLiteral("dash-plugin"),
                                                       QStringLiteral("URL:Dash Plugin Protocol (Zeal)")}};

    if (clParams.registerProtocolHandlers) {
        registerProtocolHandlers(protocols, clParams.registerProtocolHandlers);
        return EXIT_SUCCESS;
    }

    if (clParams.unregisterProtocolHandlers) {
        unregisterProtocolHandlers(protocols);
        return EXIT_SUCCESS;
    }
#endif

#ifdef Q_OS_WINDOWS
    if (clParams.attachConsole && AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *fp = nullptr;
        std::ignore = freopen_s(&fp, "CONOUT$", "w", stdout);
        std::ignore = freopen_s(&fp, "CONOUT$", "w", stderr);
        std::ignore = freopen_s(&fp, "CONIN$", "r", stdin);
    }
#endif // Q_OS_WINDOWS

    using Zeal::Core::ApplicationSingleton;
    QScopedPointer<ApplicationSingleton> appSingleton(new ApplicationSingleton());
    if (appSingleton->state() == ApplicationSingleton::State::Failed) {
        QTextStream(stderr) << "Failed to initialize application singleton." << '\n';
        return EXIT_FAILURE;
    }

    if (appSingleton->state() == ApplicationSingleton::State::Secondary) {
#ifdef Q_OS_WINDOWS
        ::AllowSetForegroundWindow(appSingleton->primaryPid());
#endif
        QByteArray ba;
        QDataStream out(&ba, QIODevice::WriteOnly);
        out << clParams.query << clParams.preventActivation;
        if (!appSingleton->sendMessage(ba)) {
            QTextStream(stderr) << "Failed to send query to the primary instance." << '\n';
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // Set application-wide window icon. All message boxes and other windows will use it by default.
    qapp->setDesktopFileName(QStringLiteral("org.zealdocs.zeal"));
    qapp->setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

    QDir::setSearchPaths(QStringLiteral("typeIcon"), {QStringLiteral(":/icons/type")});

    using Zeal::Core::Application;
    QScopedPointer<Application> app(new Application());

    QObject::connect(appSingleton.data(), &ApplicationSingleton::messageReceived, [&app](const QByteArray &data) {
        Zeal::Registry::SearchQuery query;
        bool preventActivation = false;

        QDataStream in(data);
        in >> query >> preventActivation;

        app->executeQuery(query, preventActivation);
    });

    app->showMainWindow(clParams.forceMinimized);

    if (!clParams.query.isEmpty()) {
        QTimer::singleShot(0, app.data(), [&app, clParams] {
            app->executeQuery(clParams.query, clParams.preventActivation);
        });
    }

    return qapp->exec();
}
