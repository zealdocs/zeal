#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "registry/searchquery.h"

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

class QxtGlobalShortcut;

class QSystemTrayIcon;
class QTabBar;

namespace Ui {
class MainWindow;
}

namespace Zeal {

namespace Core {
class Application;
class Settings;
}

class ListModel;
class NetworkAccessManager;
class SearchModel;
class SettingsDialog;

}

// Represents per tab search state.
// needs to contain [search input, search model, section model, url]
struct SearchState
{
    QWebPage *page;
    // model representing sections
    Zeal::SearchModel *sectionsList;
    // model representing searched for items
    Zeal::SearchModel *zealSearch;
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
    explicit MainWindow(Zeal::Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void bringToFront(const Zeal::SearchQuery &query = Zeal::SearchQuery());
    void createTab();

protected:
    void closeEvent(QCloseEvent *event) override;
    void setupShortcuts();
    void keyPressEvent(QKeyEvent *keyEvent) override;

private slots:
    void back();
    void forward();
    void onSearchComplete();
    void openDocset(const QModelIndex &index);
    void queryCompleted();
    void scrollSearch();
    void saveTabState();
    void goToTab(int index);
    void closeTab(int index = -1);

private:
    void displayViewActions();
    void loadSections(const QString &docsetName, const QUrl &url);
    void setupSearchBoxCompletions();
    void reloadTabState();
    void displayTabs();
    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;
    QAction *addHistoryAction(QWebHistory *history, QWebHistoryItem item);
    void createTrayIcon();

    QList<SearchState *> m_tabs;

    SearchState *m_searchState = nullptr;
    Zeal::NetworkAccessManager *m_zealNetworkManager = nullptr;

    Ui::MainWindow *ui = nullptr;
    Zeal::Core::Application *m_application = nullptr;
    Zeal::Core::Settings *m_settings = nullptr;
    Zeal::ListModel *m_zealListModel = nullptr;
    Zeal::SettingsDialog *m_settingsDialog = nullptr;

    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;

    bool m_treeViewClicked = false;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    QTabBar *m_tabBar = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;

#ifdef USE_LIBAPPINDICATOR
    AppIndicator *m_indicator = nullptr;  // for Unity
#endif
};

#endif // MAINWINDOW_H
