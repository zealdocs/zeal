#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocalServer>
#include <QDialog>
#include <QMenu>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QWebHistory>
#include <QModelIndex>
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

// Represents per tab search state.
// needs to contain [search input, search model, section model, url]
typedef struct SearchState
{
public:
    QWebPage *page;
    // model representing sections
    ZealSearchModel sectionsList;
    // model representing searched for items
    ZealSearchModel zealSearch;
    // query being searched for
    QString searchQuery;

    // list of selected indices
    QModelIndexList selections;
    // list of expanded indices
    QModelIndexList expansions;

    int scrollPosition;
    int sectionsScroll;
} SearchState;

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
    void loadSections(const QString docsetName, const QUrl &url);
    void setupSearchBoxCompletions();
    void createTab();
    void reloadTabState();
    void displayTabs();
    void updateTreeView(QString text);
    QIcon docsetIcon(QString docsetName);
    QAction *addHistoryAction(QWebHistory *history, QWebHistoryItem item);

    QList<SearchState*> tabs;

    SearchState *searchState;

    Ui::MainWindow *ui;
    QIcon icon;
    ZealListModel zealList;

    QLocalServer *localServer;
    QMenu backMenu;
    QMenu forwardMenu;
    QDialog hackDialog;
    bool treeViewClicked;
    void createTrayIcon();
    void setHotKey(const QKeySequence& hotKey);
    QKeySequence hotKey;
    QSettings settings;
    QTabBar tabBar;
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
    void onSearchComplete();
    void openDocset(const QModelIndex& index);
    void queryCompleted();
    void scrollSearch();
    void saveTabState();
    void goToTab(int index);
    void closeTab(int index);
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
