// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_MAINWINDOW_H
#define ZEAL_WIDGETUI_MAINWINDOW_H

#include <QMainWindow>
#include <QString>

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

    void search(const QString &query);
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
