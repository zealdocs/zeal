#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include <zealdocsetsregistry.h>

#include <iostream>
using namespace std;

#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
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
}
