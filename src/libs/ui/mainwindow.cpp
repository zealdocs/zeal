// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include "aboutdialog.h"
#include "browsertab.h"
#include "docsetsdialog.h"
#include "searchsidebar.h"
#include "settingsdialog.h"
#include "sidebarviewprovider.h"
#include <qxtglobalshortcut/qxtglobalshortcut.h>

#include <browser/settings.h>
#include <browser/webbridge.h>
#include <browser/webcontrol.h>
#include <core/application.h>
#include <core/settings.h>
#include <sidebar/container.h>
#include <sidebar/proxyview.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QVBoxLayout>

using namespace Zeal;
using namespace Zeal::WidgetUi;

MainWindow::MainWindow(Core::Application *app, QWidget *parent)
    : QMainWindow(parent)
    , m_application(app)
    , m_settings(app->settings())
{
#ifndef PORTABLE_BUILD
    setWindowTitle(tr("Zeal"));
#else
    setWindowTitle(tr("Zeal Portable"));
#endif
    resize(900, 600); // Default size. May be overridden by restoreGeometry.

    setupMainMenu();
    setupShortcuts();
    setupTabBar();

    // Setup central widget.
    auto centralWidget = new QWidget(this);
    auto centralWidgetLayout = new QVBoxLayout(centralWidget);
    centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
    centralWidgetLayout->setSpacing(0);
    centralWidgetLayout->addWidget(m_tabBar);

    m_splitter = new QSplitter(Qt::Horizontal, centralWidget);
    m_splitter->setChildrenCollapsible(false);
    centralWidgetLayout->addWidget(m_splitter);

    m_webViewStack = new QStackedWidget(m_splitter);
    m_webViewStack->setMinimumWidth(400);
    m_splitter->addWidget(m_webViewStack);

    setCentralWidget(centralWidget);

    restoreGeometry(m_settings->windowGeometry);

    // Update check
    connect(m_application, &Core::Application::updateCheckError, this, [this](const QString &message) {
        QMessageBox::warning(this, QStringLiteral("Zeal"), message);
    });

    connect(m_application, &Core::Application::updateCheckDone, this, [this](const QString &version) {
        if (version.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("Zeal"),
                                     tr("You are using the latest version."));
            return;
        }

        // TODO: Remove this ugly workaround for #637.
        qApp->setQuitOnLastWindowClosed(false);
        const int ret
                = QMessageBox::information(this, QStringLiteral("Zeal"),
                                           tr("Zeal <b>%1</b> is available. Open download page?").arg(version),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::Yes);
        qApp->setQuitOnLastWindowClosed(true);

        if (ret == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://zealdocs.org/download.html")));
        }
    });

    // Setup sidebar.
    auto m_sbViewProvider = new SidebarViewProvider(this);
    auto sbView = new Sidebar::ProxyView(m_sbViewProvider, QStringLiteral("index"));

    auto sb = new Sidebar::Container();
    sb->addView(sbView);

    // Setup splitter.
    m_splitter->insertWidget(0, sb);
    m_splitter->restoreState(m_settings->verticalSplitterGeometry);

    // Setup web settings.
    auto webSettings = new Browser::Settings(m_settings, this);

    // Setup web bridge.
    m_webBridge = new Browser::WebBridge(this);
    connect(m_webBridge, &Browser::WebBridge::actionTriggered, this, [this](const QString &action) {
        // TODO: In the future connect directly to the ActionManager.
        if (action == "openDocsetManager") {
            m_showDocsetManagerAction->trigger();
        } else if (action == "openPreferences") {
            m_showPreferencesAction->trigger();
        }
    });

    createTab();

    connect(m_settings, &Core::Settings::updated, this, &MainWindow::applySettings);
    applySettings();

    if (m_settings->checkForUpdate) {
        m_application->checkForUpdates(true);
    }
}

MainWindow::~MainWindow()
{
    m_settings->verticalSplitterGeometry = m_splitter->saveState();
    m_settings->windowGeometry = saveGeometry();
}

void MainWindow::search(const Registry::SearchQuery &query)
{
    currentTab()->search(query);
}

void MainWindow::closeTab(int index)
{
    if (index == -1) {
        index = m_tabBar->currentIndex();
    }

    if (index == -1) {
        return;
    }

    BrowserTab *tab = tabAt(index);
    m_webViewStack->removeWidget(tab);
    tab->deleteLater();

    // Handle the tab bar last to avoid currentChanged signal coming too early.
    m_tabBar->removeTab(index);

    if (m_webViewStack->count() == 0) {
        createTab();
    }
}

void MainWindow::moveTab(int from, int to)
{
    const QSignalBlocker blocker(m_webViewStack);
    QWidget *w = m_webViewStack->widget(from);
    m_webViewStack->removeWidget(w);
    m_webViewStack->insertWidget(to, w);
}

BrowserTab *MainWindow::createTab()
{
    auto tab = new BrowserTab();
    tab->navigateToStartPage();
    addTab(tab);
    return tab;
}

void MainWindow::duplicateTab(int index)
{
    BrowserTab *tab = tabAt(index);
    if (tab == nullptr)
        return;

    // Add a duplicate next to the `index`.
    addTab(tab->clone(), index + 1);
}

void MainWindow::addTab(BrowserTab *tab, int index)
{
    connect(tab, &BrowserTab::iconChanged, this, [this, tab](const QIcon &icon) {
        const int index = m_webViewStack->indexOf(tab);
        Q_ASSERT(m_tabBar->tabData(index).value<BrowserTab *>() == tab);
        m_tabBar->setTabIcon(index, icon);
    });
    connect(tab, &BrowserTab::titleChanged, this, [this, tab](const QString &title) {
        if (title.isEmpty())
            return;

#ifndef PORTABLE_BUILD
        setWindowTitle(QStringLiteral("%1 - Zeal").arg(title));
#else
        setWindowTitle(QStringLiteral("%1 - Zeal Portable").arg(title));
#endif
        const int index = m_webViewStack->indexOf(tab);
        Q_ASSERT(m_tabBar->tabData(index).value<BrowserTab *>() == tab);
        m_tabBar->setTabText(index, title);
        m_tabBar->setTabToolTip(index, title);
    });

    tab->webControl()->setWebBridgeObject("zAppBridge", m_webBridge);
    tab->searchSidebar()->focusSearchEdit();

    if (index == -1) {
        index = m_settings->openNewTabAfterActive
                ? m_tabBar->currentIndex() + 1
                : m_webViewStack->count();
    }

    m_webViewStack->insertWidget(index, tab);
    m_tabBar->insertTab(index, tr("Loading…"));
    m_tabBar->setCurrentIndex(index);
    m_tabBar->setTabData(index, QVariant::fromValue(tab));
}

BrowserTab *MainWindow::currentTab() const
{
    return tabAt(m_tabBar->currentIndex());
}

BrowserTab *MainWindow::tabAt(int index) const
{
    return qobject_cast<BrowserTab *>(m_webViewStack->widget(index));
}

void MainWindow::setupMainMenu()
{
    m_menuBar = new QMenuBar(this);

    // TODO: [Qt 6.3] Refactor using addAction(text, shortcut, receiver, member).
    // TODO: [Qt 6.7] Use QIcon::ThemeIcon.

    // File Menu.
    auto menu = m_menuBar->addMenu(tr("&File"));

    // -> New Tab Action.
    // Not a standard icon, but it is often provided by GTK themes.
    auto action = menu->addAction(
        QIcon::fromTheme(QStringLiteral("tab-new")),
        tr("New &Tab")
    );
    addAction(action);
    action->setShortcut(QKeySequence::AddTab);
    connect(action, &QAction::triggered, this, &MainWindow::createTab);

    // -> Close Tab Action.
    action = menu->addAction(tr("&Close Tab"));
    addAction(action);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_W));
    connect(action, &QAction::triggered, this, [this]() { closeTab(); });

    menu->addSeparator();

    // -> Quit Action.
    action = m_quitAction = menu->addAction(
        QIcon::fromTheme(QStringLiteral("application-exit")),
        // Follow Windows HIG.
#ifdef Q_OS_WINDOWS
        tr("E&xit"),
#else
        tr("&Quit"),
#endif
        qApp, &QApplication::quit
    );
    addAction(action);
    action->setMenuRole(QAction::QuitRole);

    // Some platform plugins do not define QKeySequence::Quit.
    if (QKeySequence(QKeySequence::Quit).isEmpty()) {
        action->setShortcut(QStringLiteral("Ctrl+Q"));
    } else {
        action->setShortcut(QKeySequence::Quit);
    }

    // Edit Menu.
    menu = m_menuBar->addMenu(tr("&Edit"));

    // -> Find in Page Action.
    action = menu->addAction(
        QIcon::fromTheme(QStringLiteral("edit-find")),
        tr("&Find in Page")
    );
    addAction(action);
    action->setShortcut(QKeySequence::Find);
    connect(action, &QAction::triggered, this, [this]() {
        if (auto tab = currentTab()) {
            tab->webControl()->activateSearchBar();
        }
    });

    menu->addSeparator();

    // -> Preferences Action.
    action = m_showPreferencesAction = menu->addAction(tr("Prefere&nces"));
    addAction(action);
    action->setMenuRole(QAction::PreferencesRole);

    if (QKeySequence(QKeySequence::Preferences).isEmpty()) {
        action->setShortcut(QStringLiteral("Ctrl+,"));
    } else {
        action->setShortcut(QKeySequence::Preferences);
    }

    connect(action, &QAction::triggered, this, [this]() {
        if (m_globalShortcut) {
            m_globalShortcut->setEnabled(false);
        }

        QScopedPointer<SettingsDialog> dialog(new SettingsDialog(this));
        dialog->exec();

        if (m_globalShortcut) {
            m_globalShortcut->setEnabled(true);
        }
    });

    // Tools Menu.
    menu = m_menuBar->addMenu(tr("&Tools"));

    // -> Docsets Action.
    m_showDocsetManagerAction = menu->addAction(
        tr("&Docsets…"),
        this, [this]() {
            QScopedPointer<DocsetsDialog> dialog(new DocsetsDialog(m_application, this));
            dialog->exec();
        }
    );

    // Help Menu.
    menu = m_menuBar->addMenu(tr("&Help"));

    // -> Submit Feedback Action.
    menu->addAction(
        tr("&Submit Feedback…"),
        this, [this]() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://go.zealdocs.org/l/report-bug")));
        }
    );

    // -> Check for Updates Action.
    menu->addAction(tr("&Check for Updates…"), this, [this]() {
        m_application->checkForUpdates(true);
    });

    menu->addSeparator();

    // -> About Action.
    action = menu->addAction(
        QIcon::fromTheme(QStringLiteral("help-about")),
        tr("&About Zeal"),
        this, [this]() {
            QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
            dialog->exec();
        }
    );
    addAction(action);
    action->setMenuRole(QAction::AboutRole);

    setMenuBar(m_menuBar);
}

void MainWindow::setupShortcuts()
{
    // Initialize the global shortcut handler if supported.
    if (QxtGlobalShortcut::isSupported()) {
        m_globalShortcut = new QxtGlobalShortcut(m_settings->showShortcut, this);
        connect(m_globalShortcut, &QxtGlobalShortcut::activated, this, &MainWindow::toggleWindow);
    }

    // Focus search bar.
    auto shortcut = new QShortcut(QStringLiteral("Ctrl+K"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->searchSidebar()->focusSearchEdit(); });

    shortcut = new QShortcut(QStringLiteral("Ctrl+L"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->searchSidebar()->focusSearchEdit(); });

    // Duplicate current tab.
    shortcut = new QShortcut(QStringLiteral("Ctrl+Alt+T"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { duplicateTab(m_tabBar->currentIndex()); });

    // Hide/show sidebar.
    // TODO: Move to the View menu.
    shortcut = new QShortcut(QStringLiteral("Ctrl+B"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto sb = m_splitter->widget(0);
        if (sb == nullptr) {
            // This should not really happen.
            return;
        }

        sb->setVisible(!sb->isVisible());
    });

    // Browser Shortcuts.
    shortcut = new QShortcut(QKeySequence::Back, this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->back(); });
    shortcut = new QShortcut(QKeySequence::Forward, this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->forward(); });
    shortcut = new QShortcut(QKeySequence::ZoomIn, this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->zoomIn(); });
    shortcut = new QShortcut(QStringLiteral("Ctrl+="), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->zoomIn(); });
    shortcut = new QShortcut(QKeySequence::ZoomOut, this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->zoomOut(); });
    shortcut = new QShortcut(QStringLiteral("Ctrl+0"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { currentTab()->webControl()->resetZoom(); });

    // TODO: Use QKeySequence::NextChild, when QTBUG-112193 is fixed.
    QAction *action = new QAction(this);
    addAction(action);
    action->setShortcuts({QKeySequence(Qt::ControlModifier | Qt::Key_Tab),
                          QKeySequence(Qt::ControlModifier | Qt::Key_PageDown)});
    connect(action, &QAction::triggered, this, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });

    // TODO: Use QKeySequence::PreviousChild, when QTBUG-15746 and QTBUG-112193 are fixed.
    action = new QAction(this);
    addAction(action);
    action->setShortcuts({QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Tab),
                          QKeySequence(Qt::ControlModifier | Qt::Key_PageUp)});
    connect(action, &QAction::triggered, this, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });
}

void MainWindow::setupTabBar()
{
    m_tabBar = new QTabBar(this);

    m_tabBar->installEventFilter(this);

    m_tabBar->setTabsClosable(true);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setStyleSheet(QStringLiteral("QTabBar::tab { width: 150px; }"));
    m_tabBar->setMovable(true);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (index == -1) {
            return;
        }

        BrowserTab *tab = tabAt(index);
#ifndef PORTABLE_BUILD
        setWindowTitle(QStringLiteral("%1 - Zeal").arg(tab->webControl()->title()));
#else
        setWindowTitle(QStringLiteral("%1 - Zeal Portable").arg(tab->webControl()->title()));
#endif

        m_webViewStack->setCurrentIndex(index);
        emit currentTabChanged();
    });
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabBar, &QTabBar::tabMoved, this, &MainWindow::moveTab);

    for (int i = 1; i < 10; i++) {
        auto action = new QAction(m_tabBar);
#ifdef Q_OS_LINUX
        action->setShortcut(QStringLiteral("Alt+%1").arg(i));
#else
        action->setShortcut(QStringLiteral("Ctrl+%1").arg(i));
#endif
        if (i == 9) {
            connect(action, &QAction::triggered, this, [=]() {
                m_tabBar->setCurrentIndex(m_tabBar->count() - 1);
            });
        } else {
            connect(action, &QAction::triggered, this, [=]() {
                m_tabBar->setCurrentIndex(i - 1);
            });
        }

        addAction(action);
    }

    connect(m_tabBar, &QTabBar::tabBarDoubleClicked, this, [this](int index) {
        if (index == -1) {
            createTab();
        }
    });
}

void MainWindow::createTrayIcon()
{
    if (m_trayIcon)
        return;

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme(QStringLiteral("zeal-tray"), windowIcon()));
    m_trayIcon->setToolTip(QStringLiteral("Zeal"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
            return;

        toggleWindow();
    });

    auto trayIconMenu = new QMenu(this);
    QAction *toggleAction = trayIconMenu->addAction(tr("Show Zeal"),
                                                    this, &MainWindow::toggleWindow);

    connect(trayIconMenu, &QMenu::aboutToShow, this, [this, toggleAction]() {
        toggleAction->setText(isVisible() ? tr("Minimize to Tray") : tr("Show Zeal"));
    });

    trayIconMenu->addSeparator();
    trayIconMenu->addAction(m_quitAction);
    m_trayIcon->setContextMenu(trayIconMenu);

    m_trayIcon->show();
}

void MainWindow::removeTrayIcon()
{
    if (!m_trayIcon)
        return;

    QMenu *trayIconMenu = m_trayIcon->contextMenu();
    delete m_trayIcon;
    m_trayIcon = nullptr;
    delete trayIconMenu;
}

void MainWindow::bringToFront()
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();

    currentTab()->searchSidebar()->focusSearchEdit();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (m_settings->showSystrayIcon && m_settings->minimizeToSystray
            && event->type() == QEvent::WindowStateChange && isMinimized()) {
        hide();
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_settings->showSystrayIcon && m_settings->hideOnClose) {
        event->ignore();
        toggleWindow();
    }
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_tabBar) {
        switch (event->type()) {
        case QEvent::MouseButtonRelease: {
            auto e = static_cast<QMouseEvent *>(event);
            if (e->button() == Qt::MiddleButton) {
                const int index = m_tabBar->tabAt(e->pos());
                if (index != -1) {
                    closeTab(index);
                    return true;
                }
            }
            break;
        }
        case QEvent::Wheel:
            // TODO: Remove in case QTBUG-8428 is fixed on all platforms
            return true;
        default:
            break;
        }
    }

    return QMainWindow::eventFilter(object, event);
}

// Captures global events in order to pass them to the search bar.
void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        currentTab()->searchSidebar()->focusSearchEdit(true);
        break;
    case Qt::Key_Question:
        currentTab()->searchSidebar()->focusSearchEdit();
        break;
    default:
        QMainWindow::keyPressEvent(keyEvent);
        break;
    }
}

void MainWindow::applySettings()
{
    if (m_globalShortcut) {
        m_globalShortcut->setShortcut(m_settings->showShortcut);
    }

    if (m_settings->showSystrayIcon)
        createTrayIcon();
    else
        removeTrayIcon();
}

void MainWindow::toggleWindow()
{
    const bool checkActive = m_globalShortcut && sender() == m_globalShortcut;

    if (!isVisible() || (checkActive && !isActiveWindow())) {
        bringToFront();
    } else {
        if (m_trayIcon) {
            hide();
        } else {
            showMinimized();
        }
    }
}
