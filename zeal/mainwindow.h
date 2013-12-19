#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocalServer>
#include <QDialog>
#include <QSettings>
#include <QSystemTrayIcon>
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include "zealnativeeventfilter.h"
#include "zealsettingsdialog.h"

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
    void bringToFrontAndSearch(const QString);

private:
    void bringToFront(bool withHack);
    void displayViewActions();

    Ui::MainWindow *ui;
    QIcon icon;
    ZealListModel zealList;
    ZealSearchModel zealSearch;
    QLocalServer *localServer;
    QDialog hackDialog;
    void createTrayIcon();
    void setHotKey(const QKeySequence& hotKey);
    QKeySequence hotKey;
    QSettings settings;
    ZealNativeEventFilter nativeFilter;
    ZealSettingsDialog settingsDialog;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QMap<QString, QString> urls;
private slots:
    void refreshRequest();
    void changeMinFontSize(int minFont);
    void back();
    void forward();
protected:
    void closeEvent(QCloseEvent *event) {
        settings.setValue("geometry", saveGeometry());
        QMainWindow::closeEvent(event);
    }
    void setupShortcuts();
};

#endif // MAINWINDOW_H
