/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
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
class SearchModel;
class SettingsDialog;

}

struct TabState
{
    QString searchQuery;

    // Content/Search results tree view state
    Zeal::SearchModel *searchModel = nullptr;
    QModelIndexList selections;
    QModelIndexList expansions;
    int searchScrollPosition;

    // TOC list view state
    Zeal::SearchModel *tocModel = nullptr;
    int tocScrollPosition;

    QWebPage *webPage = nullptr;
    int webViewZoomFactor;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Zeal::Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void search(const Zeal::SearchQuery &query);
    void bringToFront();
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
    void openDocset(const QModelIndex &index);
    void queryCompleted();
    void scrollSearch();
    void selectTab(int index);
    void closeTab(int index = -1);

private:
    void displayViewActions();
    void displayTreeView();
    void displaySections();
    void saveSectionsSplitterState();
    void setupSearchBoxCompletions();
    void setupTabBar();

    TabState *currentTabState() const;
    void saveTabState();
    void reloadTabState();

    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;
    QAction *addHistoryAction(QWebHistory *history, const QWebHistoryItem &item);

#ifdef USE_APPINDICATOR
    void detectAppIndicatorSupport();
#endif
    void createTrayIcon();
    void removeTrayIcon();

    QList<TabState *> m_tabStates;
    TabState *m_currentTabState = nullptr;

    Ui::MainWindow *ui = nullptr;
    Zeal::Core::Application *m_application = nullptr;
    Zeal::Core::Settings *m_settings = nullptr;
    Zeal::ListModel *m_zealListModel = nullptr;
    Zeal::SettingsDialog *m_settingsDialog = nullptr;

    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    QTabBar *m_tabBar = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;

#ifdef USE_APPINDICATOR
    bool m_useAppIndicator = false;
    _AppIndicator *m_appIndicator = nullptr;
    _GtkWidget *m_appIndicatorMenu = nullptr;
    _GtkWidget *m_appIndicatorQuitMenuItem = nullptr;
    _GtkWidget *m_appIndicatorShowHideMenuItem = nullptr;
    _GtkWidget *m_appIndicatorMenuSeparator = nullptr;
#endif
};

#endif // MAINWINDOW_H
