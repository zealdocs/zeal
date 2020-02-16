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

class QSystemTrayIcon;
class QTabBar;

namespace Zeal {

namespace Browser {
class WebBridge;
} // namespace Browser

namespace Core {
class Application;
class Settings;
} // namespace Core

namespace Registry {
class SearchQuery;
} //namespace Registry

namespace WidgetUi {

namespace Ui {
class MainWindow;
} // namespace Ui

class BrowserTab;
class SidebarViewProvider;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void search(const Registry::SearchQuery &query);
    void bringToFront();
    BrowserTab *createTab();

public slots:
    void toggleWindow();

signals:
    void currentTabChanged();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;

private slots:
    void applySettings();
    void closeTab(int index = -1);
    void moveTab(int from, int to);
    void duplicateTab(int index);

private:
    void setupTabBar();

    void addTab(BrowserTab *tab, int index = -1);
    BrowserTab *currentTab() const;
    BrowserTab *tabAt(int index) const;

    void createTrayIcon();
    void removeTrayIcon();

    void syncTabState(BrowserTab *tab);

    Ui::MainWindow *ui = nullptr;
    Core::Application *m_application = nullptr;
    Core::Settings *m_settings = nullptr;

    Browser::WebBridge *m_webBridge = nullptr;

    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    QTabBar *m_tabBar = nullptr;

    friend class SidebarViewProvider;
    SidebarViewProvider *m_sbViewProvider = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_MAINWINDOW_H
