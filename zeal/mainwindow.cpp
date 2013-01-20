#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include <zealdocsetsregistry.h>

#include <iostream>
using namespace std;

#include <QStandardPaths>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QLocalSocket>
#include <QDir>

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
    setWindowIcon(QIcon::fromTheme("edit-find"));
    createTrayIcon();

    // initialise key grabber
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

    // initialise docsets
    auto dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    auto dataDir = QDir(dataLocation);
    if(!dataDir.cd("docsets")) {
        QMessageBox::critical(this, "No docsets directory found",
                              QString("'docsets' directory not found in '%1'").arg(dataLocation));
    } else {
        for(auto subdir : dataDir.entryInfoList()) {
            if(subdir.isDir() && !subdir.isHidden()) {
                QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                          Q_ARG(QString, subdir.absoluteFilePath()));
            }
        }
    }

    // initialise ui
    ui->setupUi(this);
    ui->lineEdit->setTreeView(ui->treeView);
    ui->treeView->setModel(&zealList);
    ui->treeView->setColumnHidden(1, true);
    connect(ui->treeView, &QTreeView::activated, [&](const QModelIndex& index) {
        ui->webView->setUrl("file://" + index.sibling(index.row(), 1).data().toString());
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
    trayIcon->setIcon(QIcon::fromTheme("edit-find"));
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
