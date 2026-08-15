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

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
namespace {
QIcon themedTrayIcon(Core::Settings::TrayIconStyle style)
{
    // Every variant is resolved by theme name so that the tray host receives an
    // icon name rather than a pixmap. That is what lets desktops recolor the
    // automatic variant, and lets icon themes override any of them.
    switch (style) {
    case Core::Settings::TrayIconStyle::Colorful:
        return QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.svg")));
    case Core::Settings::TrayIconStyle::MonochromeLight:
        return QIcon::fromTheme(QStringLiteral("zeal-tray-light"), QIcon(QStringLiteral(":/zeal-tray-light.svg")));
    case Core::Settings::TrayIconStyle::MonochromeDark:
        return QIcon::fromTheme(QStringLiteral("zeal-tray-dark"), QIcon(QStringLiteral(":/zeal-tray-dark.svg")));
    case Core::Settings::TrayIconStyle::Automatic:
        break;
    }

    // Carries a color scheme stylesheet, so desktops that recolor tray icons tint
    // it to match the panel. Those that do not fall back to its white default,
    // hence the explicit choices above (#1950).
    return QIcon::fromTheme(QStringLiteral("zeal-tray"), QIcon(QStringLiteral(":/zeal-tray.svg")));
}
} // namespace
#endif

WindowManager::WindowManager(Core::Application *application, QObject *parent)
    : QObject(parent)
    , m_application(application)
    , m_settings(application->settings())
{
    // Update-check dialogs are session-level, not per-window.
    connect(m_application, &Core::Application::updateCheckError, this, [this](const QString &message) {
        QMessageBox::warning(activeWindow(), QStringLiteral("Zeal"), message.toHtmlEscaped());
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
                                                 tr("Zeal <b>%1</b> is available. Open download page?")
                                                     .arg(version.toHtmlEscaped()),
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
    if (!m_settings->isTrayActive()) {
        removeTrayIcon();
        return;
    }

    // createTrayIcon() applies the icon itself, so only an already visible icon
    // needs a refresh, e.g. after the style setting changed.
    if (m_trayIcon == nullptr) {
        createTrayIcon();
    } else {
        updateTrayIcon();
    }
}

void WindowManager::createTrayIcon()
{
    if (m_trayIcon != nullptr) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    updateTrayIcon();
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
        const MainWindow *target = activeWindow();
        const bool visible = target != nullptr && target->isVisible();
        toggleAction->setText(visible ? tr("Minimize to Tray") : tr("Show Zeal"));
    });

    trayIconMenu->addSeparator();
    trayIconMenu->addAction(tr("Quit"), qApp, &QApplication::quit);
    m_trayIcon->setContextMenu(trayIconMenu);

    m_trayIcon->show();
}

void WindowManager::updateTrayIcon()
{
    if (m_trayIcon == nullptr) {
        return;
    }

#ifdef Q_OS_MACOS
    // macOS menu-bar items render as template images: monochrome silhouettes
    // tinted by the system to match light/dark mode and the active accent.
    QIcon trayIcon(QStringLiteral(":/zeal-tray.svg"));
    trayIcon.setIsMask(true);
#elif defined(Q_OS_WIN)
    // Windows tray takes the icon as-is — reuse the full-color window icon.
    const QIcon trayIcon = qApp->windowIcon();
#else
    const QIcon trayIcon = themedTrayIcon(m_settings->trayIconStyle);
#endif
    m_trayIcon->setIcon(trayIcon);
}

void WindowManager::removeTrayIcon()
{
    if (m_trayIcon == nullptr) {
        return;
    }

    const QMenu *trayIconMenu = m_trayIcon->contextMenu();
    delete m_trayIcon;
    m_trayIcon = nullptr;
    delete trayIconMenu;
}

} // namespace Zeal::WidgetUi
