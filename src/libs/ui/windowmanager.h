// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WINDOWMANAGER_H
#define ZEAL_WIDGETUI_WINDOWMANAGER_H

#include <QList>
#include <QObject>

class QSystemTrayIcon;

namespace Zeal {

namespace Core {
class Application;
class Settings;
} // namespace Core

namespace Registry {
class SearchQuery;
} // namespace Registry

namespace WidgetUi {

class MainWindow;

class WindowManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WindowManager)
public:
    explicit WindowManager(Core::Application *application, QObject *parent = nullptr);
    ~WindowManager() override;

    MainWindow *openWindow(bool forceMinimized = false);
    void executeQuery(const Registry::SearchQuery &query, bool preventActivation);

private:
    MainWindow *activeWindow() const;
    MainWindow *createMainWindow();

    void applySettings();
    void createTrayIcon();
    void removeTrayIcon();

    Core::Application *m_application = nullptr;
    Core::Settings *m_settings = nullptr;

    QList<MainWindow *> m_windows;
    QSystemTrayIcon *m_trayIcon = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_WINDOWMANAGER_H
