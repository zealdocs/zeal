#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "zeallistmodel.h"
#include "zealnativeeventfilter.h"
#include "zealnetworkaccessmanager.h"
#include "zealsearchmodel.h"
#include "zealsettingsdialog.h"

#include <QCloseEvent>
#include <QDialog>
#include <QMenu>
#include <QMainWindow>
#include <QModelIndex>
#include <QLocalServer>
#include <QSettings>
#include <QSystemTrayIcon>

#ifdef USE_LIBAPPINDICATOR
#undef signals
#include <libappindicator/app-indicator.h>
#define signals public
#endif

#ifdef USE_WEBENGINE
    #include <QWebEngineHistory>
    #define QWebPage QWebEnginePage
    #define QWebHistory QWebEngineHistory
    #define QWebHistoryItem QWebEngineHistoryItem
#else
    #include <QWebHistory>
#endif

namespace Ui {
class MainWindow;
}

extern const QString serverName;

// Represents per tab search state.
// needs to contain [search input, search model, section model, url]
struct SearchState
{
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
    int zoomFactor;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void bringToFrontAndSearch(const QString &query);
    void createTab();
    bool startHidden();

protected:
    void closeEvent(QCloseEvent *event)
    {
        settings.setValue("geometry", saveGeometry());
        event->ignore();
        hide();
    }

    void setupShortcuts();
    void keyPressEvent(QKeyEvent *keyEvent);

private slots:
    void refreshRequest();
    void changeMinFontSize(int minFont);
    void back();
    void forward();
    void onSearchComplete();
    void openDocset(const QModelIndex &index);
    void queryCompleted();
    void scrollSearch();
    void saveTabState();
    void goToTab(int index);
    void closeTab(int index = -1);
    void applyWebPageStyle();

private:
    void bringToFront(bool withHack);
    void displayViewActions();
    void loadSections(const QString &docsetName, const QUrl &url);
    void setupSearchBoxCompletions();
    void reloadTabState();
    void displayTabs();
    QIcon docsetIcon(const QString &docsetName);
    QAction *addHistoryAction(QWebHistory *history, QWebHistoryItem item);
    void createTrayIcon();
    void setHotKey(const QKeySequence &hotKey);

    QList<SearchState *> tabs;

    SearchState *searchState;
    ZealNetworkAccessManager *zealNaManager;

    Ui::MainWindow *ui;
    QIcon icon;
    ZealListModel zealList;

    QLocalServer *localServer;
    QMenu backMenu;
    QMenu forwardMenu;
    QDialog hackDialog;
    bool treeViewClicked;

    QKeySequence hotKey;
    QSettings settings;
    QTabBar tabBar;
    ZealNativeEventFilter nativeFilter;
    ZealSettingsDialog settingsDialog;
    QSystemTrayIcon *trayIcon;

#ifdef USE_LIBAPPINDICATOR
    AppIndicator *indicator;  // for Unity
#endif

    QMenu *trayIconMenu;
    QMap<QString, QString> urls;
    QString getDocsetName(const QString &urlPath);
};

#endif // MAINWINDOW_H
