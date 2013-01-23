#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include "zealnativeeventfilter.h"
#include "zealdocsetsregistry.h"

#include <QDebug>
#include <QAbstractEventDispatcher>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QLocalSocket>
#include <QDir>

#ifdef WIN32
#include <windows.h>
#endif

const QString serverName = "zeal_process_running";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // server for detecting already running instances
    localServer = new QLocalServer(this);
    connect(localServer, &QLocalServer::newConnection, [&]() {
        bringToFront();
    });
    QLocalServer::removeServer(serverName);  // remove in case previous instance crashed
    localServer->listen(serverName);

    // initialise icons
#ifdef WIN32
    icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation);
#else
    icon = QIcon::fromTheme("edit-find");
#endif
    setWindowIcon(icon);
    createTrayIcon();

    // initialise key grabber
#ifdef WIN32
    auto filter = new ZealNativeEventFilter();
    connect(filter, &ZealNativeEventFilter::gotHotKey, [&]() {
        if(isVisible()) hide();
        else {
            bringToFront();
        }
    });
    qApp->eventDispatcher()->installNativeEventFilter(filter);
    RegisterHotKey(NULL, 10, MOD_ALT, VK_SPACE);
#else
    keyGrabber.setParent(this);
    keyGrabber.start(qApp->applicationFilePath(), {"--grabkey"});
    connect(&keyGrabber, &QProcess::readyRead, [&]() {
        char buf[100];
        keyGrabber.readLine(buf, sizeof(buf));
        if(QString(buf) == "failed\n") {
            QMessageBox::warning(this, "grabkey process init failed",
                                 QString("Failed to grab keyboard - Alt+Space will not work."));
        }
        //first = false;
        if(QString(buf) == "1\n") {
            if(isVisible()) hide();
            else {
                bringToFront();
            }
        }
    });
#endif  // WIN32
    // initialise docsets
    auto dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    auto dataDir = QDir(dataLocation);
    if(!dataDir.cd("docsets")) {
        QMessageBox::critical(this, "No docsets directory found",
                              QString("'docsets' directory not found in '%1'").arg(dataLocation));
    } else {
        for(auto subdir : dataDir.entryInfoList()) {
            if(subdir.isDir() && subdir.fileName() != "." && subdir.fileName() != "..") {
                QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                          Q_ARG(QString, subdir.absoluteFilePath()));
            }
        }
    }

    // initialise ui
    ui->setupUi(this);

    // menu
    auto quitAction = ui->menuBar->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    auto helpMenu = new QMenu("&Help");
    auto aboutAction = helpMenu->addAction("&About");
    auto aboutQtAction = helpMenu->addAction("About &Qt");
    connect(aboutAction, &QAction::triggered,
            [&]() { QMessageBox::about(this, "About Zeal",
                "This is Zeal - a documentation browser.\n\n"
                "For details see https://github.com/jkozera/zeal/"); });
    connect(aboutQtAction, &QAction::triggered,
            [&]() { QMessageBox::aboutQt(this); });
    ui->menuBar->addMenu(helpMenu);

    // treeView and lineEdit
    ui->lineEdit->setTreeView(ui->treeView);
    ui->treeView->setModel(&zealList);
    ui->treeView->setColumnHidden(1, true);
    connect(ui->treeView, &QTreeView::activated, [&](const QModelIndex& index) {
        QStringList url_l = index.sibling(index.row(), 1).data().toString().split('#');
        QUrl url = QUrl::fromLocalFile(url_l[0]);
        if(url_l.count() > 1) {
            url.setFragment(url_l[1]);
        }
        ui->webView->load(url);
    });
    connect(&zealSearch, &ZealSearchModel::queryCompleted, [&]() {
        ui->treeView->setModel(&zealSearch);
        ui->treeView->reset();
        ui->treeView->setColumnHidden(1, true);
        ui->treeView->setCurrentIndex(zealSearch.index(0, 0, QModelIndex()));
    });
    connect(ui->lineEdit, &QLineEdit::textChanged, [&](const QString& text) {
        if(!text.isEmpty()) {
            zealSearch.setQuery(text);
        } else {
            ui->treeView->setModel(&zealList);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete localServer;
    keyGrabber.terminate();
    keyGrabber.waitForFinished(500);
}

void MainWindow::createTrayIcon()
{
    auto trayIconMenu = new QMenu(this);
    auto quitAction = trayIconMenu->addAction("&Quit");
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    auto trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("Zeal");
    connect(trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if(reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if(isVisible()) hide();
            else show();
        }
    });
    trayIcon->show();
}

void MainWindow::bringToFront()
{
    show();
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();
}
