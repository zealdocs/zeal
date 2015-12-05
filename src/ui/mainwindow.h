/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "registry/searchquery.h"
#include "registry/cancellationtoken.h"

#include <QDialog>
#include <QMainWindow>
#include <QModelIndex>

#ifdef USE_WEBENGINE
#define QWebPage QWebEnginePage
#define QWebHistory QWebEngineHistory
#define QWebHistoryItem QWebEngineHistoryItem
#endif

#ifdef USE_APPINDICATOR
struct _AppIndicator;
struct _GtkWidget;
#endif

class QxtGlobalShortcut;

class QSystemTrayIcon;
class QTabBar;
class QWebHistory;
class QWebHistoryItem;
class QWebPage;

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

public slots:
    void toggleWindow();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;

private slots:
    void applySettings();
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
    void setupSearchBoxCompletions();
    void reloadTabState();
    void displayTabs();
    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;
    QAction *addHistoryAction(QWebHistory *history, const QWebHistoryItem &item);
    void createTrayIcon();
    void removeTrayIcon();

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

    Zeal::CancellationToken m_cancelSearch;
    bool m_treeViewClicked = false;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    QTabBar *m_tabBar = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;

#ifdef USE_APPINDICATOR
    _AppIndicator *m_appIndicator = nullptr;
    _GtkWidget *m_appIndicatorMenu = nullptr;
    _GtkWidget *m_appIndicatorQuitMenuItem = nullptr;
    _GtkWidget *m_appIndicatorShowHideMenuItem = nullptr;
    _GtkWidget *m_appIndicatorMenuSeparator = nullptr;
#endif
};

#endif // MAINWINDOW_H
