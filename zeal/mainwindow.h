#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocalServer>
#include <QDialog>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include "zealnativeeventfilter.h"
#include "zealsettingsdialog.h"

#ifdef USE_LIBAPPINDICATOR
#undef signals
#include <libappindicator/app-indicator.h>
#define signals public
#endif


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
    bool startHidden();

private:
    void bringToFront(bool withHack);
    void displayViewActions();
    void setupSearchBoxCompletions();

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
#ifdef USE_LIBAPPINDICATOR
    AppIndicator *indicator;  //for Unity
#endif
    QMenu *trayIconMenu;
    QMap<QString, QString> urls;
    QString getDocsetName(QString urlPath);
private slots:
    void refreshRequest();
    void changeMinFontSize(int minFont);
    void back();
    void forward();
protected:
    void closeEvent(QCloseEvent *event) {
        settings.setValue("geometry", saveGeometry());
        event->ignore();
        hide();
    }
    void setupShortcuts();
    void keyPressEvent(QKeyEvent *keyEvent);
};

#endif // MAINWINDOW_H
