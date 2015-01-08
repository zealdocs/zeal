#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "zeallistmodel.h"
#include "zealnativeeventfilter.h"
#include "zealsearchmodel.h"
#include "zealsettingsdialog.h"

#include <QDialog>
#include <QMainWindow>
#include <QModelIndex>

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

class QLocalServer;
class QSettings;
class QSystemTrayIcon;

namespace Ui {
class MainWindow;
}

namespace Zeal {
class NetworkAccessManager;
}

extern const QString serverName;

// Represents per tab search state.
// needs to contain [search input, search model, section model, url]
struct SearchState
{
    QWebPage *page;
    // model representing sections
    Zeal::SearchModel sectionsList;
    // model representing searched for items
    Zeal::SearchModel zealSearch;
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
    void closeEvent(QCloseEvent *event) override;
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
    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;
    QAction *addHistoryAction(QWebHistory *history, QWebHistoryItem item);
    void createTrayIcon();
    void setHotKey(const QKeySequence &m_hotKey);

    QList<SearchState *> m_tabs;

    SearchState *m_searchState = nullptr;
    Zeal::NetworkAccessManager *m_zealNetworkManager = nullptr;

    Ui::MainWindow *ui = nullptr;
    QSettings *m_settings = nullptr;
    Zeal::ListModel *m_zealListModel = nullptr;
    Zeal::SettingsDialog *m_settingsDialog = nullptr;

    QIcon m_icon;

    QLocalServer *m_localServer = nullptr;

    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;

    QDialog m_hackDialog;
    bool m_treeViewClicked;

    QKeySequence m_hotKey;
    QTabBar m_tabBar;
    Zeal::NativeEventFilter m_nativeFilter;

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayIconMenu = nullptr;

#ifdef USE_LIBAPPINDICATOR
    AppIndicator *m_indicator = nullptr;  // for Unity
#endif
};

#endif // MAINWINDOW_H
