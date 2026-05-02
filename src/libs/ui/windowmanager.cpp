// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowmanager.h"

#include "mainwindow.h"

#include <core/application.h>
#include <core/settings.h>
#include <registry/searchquery.h>

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QUrl>

namespace Zeal::WidgetUi {

WindowManager::WindowManager(Core::Application *application, QObject *parent)
    : QObject(parent)
    , m_application(application)
    , m_settings(application->settings())
{
    // Update-check dialogs are session-level, not per-window.
    connect(m_application, &Core::Application::updateCheckError, this, [this](const QString &message) {
        QMessageBox::warning(activeWindow(), QStringLiteral("Zeal"), message);
    });

    connect(m_application, &Core::Application::updateCheckDone, this, [this](const QString &version) {
        if (version.isEmpty()) {
            QMessageBox::information(activeWindow(), QStringLiteral("Zeal"), tr("You are using the latest version."));
            return;
        }

        // TODO: Remove this ugly workaround for #637.
        qApp->setQuitOnLastWindowClosed(false);
        const int ret = QMessageBox::information(activeWindow(),
                                                 QStringLiteral("Zeal"),
                                                 tr("Zeal <b>%1</b> is available. Open download page?").arg(version),
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::Yes);
        qApp->setQuitOnLastWindowClosed(true);

        if (ret == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://zealdocs.org/download.html")));
        }
    });

    connect(m_settings, &Core::Settings::updated, this, &WindowManager::applySettings);
    applySettings();

    if (m_settings->checkForUpdate) {
        m_application->checkForUpdates(true);
    }
}

WindowManager::~WindowManager()
{
    removeTrayIcon();
    qDeleteAll(m_windows);
}

MainWindow *WindowManager::openWindow(bool forceMinimized)
{
    auto *w = createMainWindow();

    if (forceMinimized || m_settings->startMinimized) {
        if (!m_settings->isTrayActive() || !m_settings->minimizeToSystray) {
            w->showMinimized();
        }
    } else {
        w->show();
    }

    return w;
}

MainWindow *WindowManager::createMainWindow()
{
    auto *w = new MainWindow(m_application);
    m_windows.append(w);
    return w;
}

void WindowManager::executeQuery(const Registry::SearchQuery &query, bool preventActivation)
{
    MainWindow *target = activeWindow();
    if (target == nullptr) {
        target = openWindow();
    }

    target->search(query);
    if (!preventActivation) {
        target->bringToFront();
    }
}

MainWindow *WindowManager::activeWindow() const
{
    if (m_windows.isEmpty()) {
        return nullptr;
    }

    for (MainWindow *w : m_windows) {
        if (w->isActiveWindow()) {
            return w;
        }
    }

    return m_windows.first();
}

void WindowManager::applySettings()
{
    if (m_settings->isTrayActive()) {
        createTrayIcon();
    } else {
        removeTrayIcon();
    }
}

void WindowManager::createTrayIcon()
{
    if (m_trayIcon) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme(QStringLiteral("zeal-tray"), qApp->windowIcon()));
    m_trayIcon->setToolTip(QStringLiteral("Zeal"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick) {
            return;
        }

        MainWindow *target = activeWindow();
        if (target == nullptr) {
            createMainWindow()->bringToFront();
            return;
        }

        target->toggleWindow();
    });

    auto *trayIconMenu = new QMenu();
    QAction *toggleAction = trayIconMenu->addAction(tr("Show Zeal"), this, [this]() {
        MainWindow *target = activeWindow();
        if (target == nullptr) {
            createMainWindow()->bringToFront();
            return;
        }

        target->toggleWindow();
    });

    connect(trayIconMenu, &QMenu::aboutToShow, this, [this, toggleAction]() {
        MainWindow *target = activeWindow();
        const bool visible = target != nullptr && target->isVisible();
        toggleAction->setText(visible ? tr("Minimize to Tray") : tr("Show Zeal"));
    });

    trayIconMenu->addSeparator();
    trayIconMenu->addAction(tr("Quit"), qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(trayIconMenu);

    m_trayIcon->show();
}

void WindowManager::removeTrayIcon()
{
    if (!m_trayIcon) {
        return;
    }

    QMenu *trayIconMenu = m_trayIcon->contextMenu();
    delete m_trayIcon;
    m_trayIcon = nullptr;
    delete trayIconMenu;
}

} // namespace Zeal::WidgetUi
