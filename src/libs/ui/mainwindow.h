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

#ifndef ZEAL_WIDGETUI_MAINWINDOW_H
#define ZEAL_WIDGETUI_MAINWINDOW_H

#include <QMainWindow>

class QxtGlobalShortcut;

class QModelIndex;
class QSystemTrayIcon;
class QTabBar;
class QTimer;

namespace Zeal {

namespace Core {
class Application;
class Settings;
} // namespace Core

namespace Registry {
class ListModel;
class SearchQuery;
} //namespace Registry

namespace WidgetUi {

namespace Ui {
class MainWindow;
} // namespace Ui

class WebBridge;
class WebViewTab;

struct TabState;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void search(const Registry::SearchQuery &query);
    void bringToFront();
    WebViewTab *createTab(int index = -1);

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
    void moveTab(int from, int to);
    void duplicateTab(int index);

private:
    void syncTreeView();
    void syncToc();
    void setupSearchBoxCompletions();
    void setupTabBar();

    TabState *currentTabState() const;
    WebViewTab *currentTab() const;

    void attachTab(TabState *tabState);
    void detachTab(TabState *tabState);

    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QString &docsetName) const;

    void createTrayIcon();
    void removeTrayIcon();

    void syncTabState(TabState *tabState);

    QList<TabState *> m_tabStates;

    Ui::MainWindow *ui = nullptr;
    Core::Application *m_application = nullptr;
    Core::Settings *m_settings = nullptr;
    Registry::ListModel *m_zealListModel = nullptr;

    WebBridge *m_webBridge = nullptr;

    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    QTabBar *m_tabBar = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;

    QTimer *m_openDocsetTimer = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_MAINWINDOW_H
