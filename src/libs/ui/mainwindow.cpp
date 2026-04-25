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

#include <browser/settings.h>
#include <browser/webbridge.h>
#include <browser/webcontrol.h>
#include <core/application.h>
#include <core/session.h>
#include <core/settings.h>
#include <qxtglobalshortcut/qxtglobalshortcut.h>
#include <sidebar/container.h>
#include <sidebar/proxyview.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace Zeal::WidgetUi {

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
    auto *centralWidget = new QWidget(this);
    auto *centralWidgetLayout = new QVBoxLayout(centralWidget);
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

    Core::WindowState &windowState = m_application->session()->primaryWindow();
    restoreGeometry(windowState.geometry);

    // Setup sidebar.
    auto *sbViewProvider = new SidebarViewProvider(this);
    auto *sbView = new Sidebar::ProxyView(sbViewProvider, QStringLiteral("index"));

    auto *sb = new Sidebar::Container();
    sb->addView(sbView);

    // Setup splitter.
    m_splitter->insertWidget(0, sb);
    m_splitter->restoreState(windowState.splitterState);

    // Setup web settings.
    new Browser::Settings(m_settings, this);

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
}

MainWindow::~MainWindow()
{
    Core::WindowState &windowState = m_application->session()->primaryWindow();
    windowState.splitterState = m_splitter->saveState();
    windowState.geometry = saveGeometry();
}

void MainWindow::search(const Registry::SearchQuery &query)
{
    if (auto *tab = currentTab()) {
        tab->search(query);
    }
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

BrowserTab *MainWindow::createTab(const QUrl &url, bool activate)
{
    BrowserTab *tab;
    if (url.isEmpty()) {
        tab = new BrowserTab();
    } else {
        auto *source = currentTab();
        // Clone the current tab to preserve sidebar state.
        tab = source ? source->clone(false) : new BrowserTab();
    }

    // Add the tab before navigating so signal handlers are connected.
    addTab(tab, -1, activate);

    if (url.isEmpty()) {
        tab->navigateToStartPage();
        if (activate) {
            tab->searchSidebar()->focusSearchEdit();
        }
    } else {
        tab->webControl()->load(url);
        if (activate) {
            tab->webControl()->focus();
        }
    }

    return tab;
}

void MainWindow::duplicateTab(int index)
{
    BrowserTab *tab = tabAt(index);
    if (tab == nullptr) {
        return;
    }

    // Add a duplicate next to the `index`.
    addTab(tab->clone(), index + 1);
}

void MainWindow::addTab(BrowserTab *tab, int index, bool activate)
{
    connect(tab, &BrowserTab::iconChanged, this, [this, tab](const QIcon &icon) {
        const int index = m_webViewStack->indexOf(tab);
        Q_ASSERT(m_tabBar->tabData(index).value<BrowserTab *>() == tab);
        m_tabBar->setTabIcon(index, icon);
    });
    connect(tab, &BrowserTab::titleChanged, this, [this, tab](const QString &title) {
        if (title.isEmpty()) {
            return;
        }

        const int index = m_webViewStack->indexOf(tab);
        Q_ASSERT(m_tabBar->tabData(index).value<BrowserTab *>() == tab);
        m_tabBar->setTabText(index, title);
        m_tabBar->setTabToolTip(index, title);

        // Only update window title for the active tab.
        if (tab == currentTab()) {
#ifndef PORTABLE_BUILD
            setWindowTitle(QStringLiteral("%1 - Zeal").arg(title));
#else
            setWindowTitle(QStringLiteral("%1 - Zeal Portable").arg(title));
#endif
        }
    });

    tab->webControl()->setWebBridgeObject("zAppBridge", m_webBridge);

    connect(tab->searchSidebar(), &SearchSidebar::openInNewTabRequested, this, [this](const QUrl &url, bool activate) {
        createTab(url, activate);
    });

    if (index == -1) {
        index = m_settings->openNewTabAfterActive ? m_tabBar->currentIndex() + 1 : m_webViewStack->count();
    }

    m_webViewStack->insertWidget(index, tab);
    m_tabBar->insertTab(index, tr("Loading…"));
    m_tabBar->setTabData(index, QVariant::fromValue(tab));

    if (activate) {
        m_tabBar->setCurrentIndex(index);
    }
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
    m_menuBar->installEventFilter(this);

    // TODO: [Qt 6.3] Refactor using addAction(text, shortcut, receiver, member).
    // TODO: [Qt 6.7] Use QIcon::ThemeIcon.

    // File Menu.
    auto *menu = m_menuBar->addMenu(tr("&File"));

    // -> New Tab Action.
    // Not a standard icon, but it is often provided by GTK themes.
    auto *action = menu->addAction(QIcon::fromTheme(QStringLiteral("tab-new")), tr("New &Tab"));
    addAction(action);
    action->setShortcut(QKeySequence::AddTab);
    connect(action, &QAction::triggered, this, [this]() {
        createTab();
    });

    // -> Close Tab Action.
    action = menu->addAction(tr("&Close Tab"));
    addAction(action);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_W));
    connect(action, &QAction::triggered, this, [this]() {
        closeTab();
    });

    menu->addSeparator();

    // -> Quit Action.
    // Follow Windows HIG naming.
    action = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")),
#ifdef Q_OS_WINDOWS
                             tr("E&xit"),
#else
                             tr("&Quit"),
#endif
                             qApp,
                             &QApplication::quit);
    addAction(action);
    action->setMenuRole(QAction::QuitRole);
    action->setShortcut(QStringLiteral("Ctrl+Q"));

    // Edit Menu.
    menu = m_menuBar->addMenu(tr("&Edit"));

    // -> Find in Page Action.
    action = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), tr("&Find in Page"));
    addAction(action);
    action->setShortcut(QKeySequence::Find);
    connect(action, &QAction::triggered, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->activateSearchBar();
        }
    });

    menu->addSeparator();

    // -> Preferences Action.
    // cspell:disable-next-line - cSpell does not like the ampersand.
    action = m_showPreferencesAction = menu->addAction(tr("Prefere&nces"));
    addAction(action);
    action->setMenuRole(QAction::PreferencesRole);
    action->setShortcut(QStringLiteral("Ctrl+,"));

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

    // View Menu.
    menu = m_menuBar->addMenu(tr("&View"));

    // Menu bar is global on MacOS, so it should always be visible.
#ifndef Q_OS_MACOS
    // -> Toolbars Submenu.
    auto *subMenu = menu->addMenu(tr("&Toolbars"));

    // -> Toggle Toolbar Action.
    action = m_showMenuBarAction = subMenu->addAction(tr("&Menu Bar"));
    addAction(action);
    action->setCheckable(true);
    action->setChecked(!m_settings->hideMenuBar);
    action->setShortcut(QKeySequence(QStringLiteral("Ctrl+M")));
    connect(action, &QAction::toggled, this, [this](bool checked) {
        m_menuBar->setVisible(checked);
        m_settings->hideMenuBar = !checked;
        m_settings->save();
    });

    // Set menu bar visibility.
    m_menuBar->setVisible(m_showMenuBarAction->isChecked());

    // Show and focus menu bar on F10.
    auto *focusMenu = new QShortcut(Qt::Key_F10, this);
    connect(focusMenu, &QShortcut::activated, this, [this]() {
        m_menuBar->setVisible(true);

        m_menuBar->setFocus();
        if (!m_menuBar->actions().isEmpty()) {
            m_menuBar->setActiveAction(m_menuBar->actions().first());
        }
    });

    menu->addSeparator();
#endif

    // -> Zoom Submenu.
    auto *zoomMenu = menu->addMenu(tr("&Zoom"));

    // -> -> Zoom In Action.
    action = zoomMenu->addAction(QIcon::fromTheme(QStringLiteral("zoom-in")), tr("Zoom &In"));
    addAction(action);
    action->setShortcuts({QKeySequence::ZoomIn, QKeySequence(QStringLiteral("Ctrl+="))});
    connect(action, &QAction::triggered, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->zoomIn();
        }
    });

    // -> -> Zoom Out Action.
    action = zoomMenu->addAction(QIcon::fromTheme(QStringLiteral("zoom-out")), tr("Zoom &Out"));
    addAction(action);
    action->setShortcut(QKeySequence::ZoomOut);
    connect(action, &QAction::triggered, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->zoomOut();
        }
    });

    zoomMenu->addSeparator();

    // -> -> Actual Size Action.
    action = zoomMenu->addAction(QIcon::fromTheme(QStringLiteral("zoom-original")), tr("&Actual Size"));
    addAction(action);
    action->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(action, &QAction::triggered, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->resetZoom();
        }
    });

    // Tools Menu.
    menu = m_menuBar->addMenu(tr("&Tools"));

    // -> Docsets Action.
    m_showDocsetManagerAction = menu->addAction(tr("&Docsets…"), this, [this]() {
        QScopedPointer<DocsetsDialog> dialog(new DocsetsDialog(m_application, this));
        dialog->exec();
    });

    // Help Menu.
    menu = m_menuBar->addMenu(tr("&Help"));

    // -> Submit Feedback Action.
    menu->addAction(tr("&Submit Feedback…"), this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://go.zealdocs.org/l/report-bug")));
    });

    // -> Check for Updates Action.
    menu->addAction(tr("&Check for Updates…"), this, [this]() {
        m_application->checkForUpdates();
    });

    menu->addSeparator();

    // -> About Action.
    action = menu->addAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("&About Zeal"), this, [this]() {
        QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
        dialog->exec();
    });
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
    auto *shortcut = new QShortcut(QStringLiteral("Ctrl+K"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->searchSidebar()->focusSearchEdit();
        }
    });

    shortcut = new QShortcut(QStringLiteral("Ctrl+L"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->searchSidebar()->focusSearchEdit();
        }
    });

    // Duplicate current tab.
    shortcut = new QShortcut(QStringLiteral("Ctrl+Alt+T"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        duplicateTab(m_tabBar->currentIndex());
    });

    // Hide/show sidebar.
    // TODO: Move to the View menu.
    shortcut = new QShortcut(QStringLiteral("Ctrl+B"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto *sb = m_splitter->widget(0);
        if (sb == nullptr) {
            // This should not really happen.
            return;
        }

        sb->setVisible(!sb->isVisible());
    });

    // Browser Shortcuts.
    shortcut = new QShortcut(QKeySequence::Back, this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->back();
        }
    });
    shortcut = new QShortcut(QKeySequence::Forward, this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (auto *tab = currentTab()) {
            tab->webControl()->forward();
        }
    });

    // TODO: Use QKeySequence::NextChild, when QTBUG-112193 is fixed.
    auto *action = new QAction(this);
    addAction(action);
    action->setShortcuts(
        {QKeySequence(Qt::ControlModifier | Qt::Key_Tab), QKeySequence(Qt::ControlModifier | Qt::Key_PageDown)});
    connect(action, &QAction::triggered, this, [this]() {
        const int count = m_tabBar->count();
        if (count > 0) {
            m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % count);
        }
    });

    // TODO: Use QKeySequence::PreviousChild, when QTBUG-15746 and QTBUG-112193 are fixed.
    action = new QAction(this);
    addAction(action);
    action->setShortcuts({QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Tab),
                          QKeySequence(Qt::ControlModifier | Qt::Key_PageUp)});
    connect(action, &QAction::triggered, this, [this]() {
        const int count = m_tabBar->count();
        if (count > 0) {
            m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + count) % count);
        }
    });
}

void MainWindow::setupTabBar()
{
    m_tabBar = new QTabBar(this);
    m_tabBar->installEventFilter(this);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setExpanding(false);
    m_tabBar->setMovable(true);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    m_tabBar->setStyleSheet(QStringLiteral("QTabBar::tab { width: 150px; }"));
    m_tabBar->setTabsClosable(m_settings->showTabCloseButton);
    m_tabBar->setUsesScrollButtons(true);

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
        auto *action = new QAction(m_tabBar);
#ifdef Q_OS_LINUX
        action->setShortcut(QStringLiteral("Alt+%1").arg(i));
#else
        action->setShortcut(QStringLiteral("Ctrl+%1").arg(i));
#endif
        if (i == 9) {
            connect(action, &QAction::triggered, this, [this]() {
                m_tabBar->setCurrentIndex(m_tabBar->count() - 1);
            });
        } else {
            connect(action, &QAction::triggered, this, [this, i]() {
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

void MainWindow::bringToFront()
{
#ifdef Q_OS_WINDOWS
    // On Windows, show() + setWindowState() causes the shell to fire resize
    // events that reset the window geometry to its pre-snap size, discarding
    // any snapped or manually positioned state. To work around this, we save
    // the geometry before hiding and restore it after the shell has finished
    // its resize pass (deferred via a zero-delay timer). The window is kept
    // transparent during the transition to avoid a visible flicker.
    // See: https://github.com/zealdocs/zeal/issues/699
    // See: https://bugreports.qt.io/browse/QTBUG-59668
    if (!m_savedGeometry.isEmpty()) {
        setWindowOpacity(0.0);
    }
#endif
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized);
#ifdef Q_OS_WINDOWS
    if (!m_savedGeometry.isEmpty()) {
        const QByteArray geom = m_savedGeometry;
        m_savedGeometry.clear();
        QTimer::singleShot(0, this, [this, geom]() {
            restoreGeometry(geom);
            setWindowOpacity(1.0);
        });
    }
#endif
    activateWindow();
    raise();

    // Use _NET_ACTIVE_WINDOW on X11 to bypass focus-stealing prevention.
    if (QWindow *w = windowHandle()) {
        w->requestActivate();
    }

    if (auto *tab = currentTab()) {
        tab->searchSidebar()->focusSearchEdit();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (m_settings->isTrayActive() && m_settings->minimizeToSystray && event->type() == QEvent::WindowStateChange
        && isMinimized()) {
        hide();
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_settings->isTrayActive() && m_settings->hideOnClose) {
        event->ignore();
        toggleWindow();
    }
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_tabBar) {
        switch (event->type()) {
        case QEvent::MouseButtonRelease: {
            auto *e = static_cast<QMouseEvent *>(event);
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

#ifndef Q_OS_MACOS
    if (object == m_menuBar && m_menuBar->isVisible() && m_showMenuBarAction != nullptr
        && !m_showMenuBarAction->isChecked()) {
        switch (event->type()) {
        // Hide menu bar when it loses focus.
        case QEvent::FocusOut: {
            auto *e = static_cast<QFocusEvent *>(event);
            if (e->reason() != Qt::PopupFocusReason) {
                m_menuBar->hide();
            }

            break;
        }
        // Hide menu bar on Escape key press.
        case QEvent::KeyPress: {
            auto *e = static_cast<QKeyEvent *>(event);
            if (e->key() == Qt::Key_Escape) {
                m_menuBar->hide();
            }

            break;
        }

        default:
            break;
        }
    }
#endif

    return QMainWindow::eventFilter(object, event);
}

// Captures global events in order to pass them to the search bar.
void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        if (auto *tab = currentTab()) {
            tab->searchSidebar()->focusSearchEdit(true);
        }
        break;
    case Qt::Key_Question:
        if (auto *tab = currentTab()) {
            tab->searchSidebar()->focusSearchEdit();
        }
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

    m_tabBar->setTabsClosable(m_settings->showTabCloseButton);
}

void MainWindow::toggleWindow()
{
    const bool checkActive = m_globalShortcut && sender() == m_globalShortcut;

    if (!isVisible() || (checkActive && !isActiveWindow())) {
        bringToFront();
    } else {
        if (m_settings->isTrayActive()) {
#ifdef Q_OS_WINDOWS
            m_savedGeometry = saveGeometry(); // See bringToFront().
#endif
            hide();
        } else {
            showMinimized();
        }
    }
}

} // namespace Zeal::WidgetUi
