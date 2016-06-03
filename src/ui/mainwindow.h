/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "registry/searchquery.h"

#include <QMainWindow>

#ifdef USE_WEBENGINE
class QWebEngineHistory;
class QWebEngineHistoryItem;
class QWebEnginePage;
typedef QWebEngineHistory QWebHistory;
typedef QWebEngineHistoryItem QWebHistoryItem;
typedef QWebEnginePage QWebPage;
#else
class QWebHistory;
class QWebHistoryItem;
class QWebPage;
#endif

#ifdef USE_APPINDICATOR
struct _AppIndicator;
struct _GtkWidget;
#endif

class QxtGlobalShortcut;

class QModelIndex;
class QSystemTrayIcon;
class QTabBar;

namespace Ui {
class MainWindow;
} // namespace Ui

namespace Zeal {

namespace Core {
class Application;
class Settings;
} // namespace Core

class ListModel;
} // namespace Zeal

struct TabState;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Zeal::Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void search(const Zeal::SearchQuery &query);
    void bringToFront();
    void createTab(int index = -1);

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
    void closeTab(int index = -1);

private:
    void displayViewActions();
    void displayTreeView();
    void toggleToc();
    void setupSearchBoxCompletions();
    void setupTabBar();
    void restoreLastSession();
    void saveSession();

    TabState *currentTabState() const;

    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;
    QAction *addHistoryAction(QWebHistory *history, const QWebHistoryItem &item);

#ifdef USE_APPINDICATOR
    void detectAppIndicatorSupport();
#endif
    void createTrayIcon();
    void removeTrayIcon();

    QList<TabState *> m_tabStates;

    Ui::MainWindow *ui = nullptr;
    Zeal::Core::Application *m_application = nullptr;
    Zeal::Core::Settings *m_settings = nullptr;
    Zeal::ListModel *m_zealListModel = nullptr;

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
