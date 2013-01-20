#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QLocalServer>
#include "zeallistmodel.h"
#include "zealsearchmodel.h"

namespace Ui {
class MainWindow;
}

extern const QString serverName;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    void bringToFront();
    Ui::MainWindow *ui;
    ZealListModel zealList;
    ZealSearchModel zealSearch;
    QProcess keyGrabber;
    QLocalServer *localServer;
    void createTrayIcon();
};

#endif // MAINWINDOW_H
