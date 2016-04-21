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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "searchitemdelegate.h"
#include "settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetregistry.h"
#include "registry/listmodel.h"
#include "registry/searchmodel.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTabBar>

#ifdef USE_WEBENGINE
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineSettings>

typedef QWebEngineHistory QWebHistory;
#else
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#endif

#include <qxtglobalshortcut.h>

// TODO: [Qt 5.5] Remove in favour of native Qt support (QTBUG-31762)
#ifdef USE_APPINDICATOR
#undef signals
#include <libappindicator/app-indicator.h>
#define signals public
#include <gtk/gtk.h>
#endif

using namespace Zeal;

namespace {
const char startPageUrl[] = "qrc:///browser/start.html";
}

struct TabState
{
    explicit TabState()
    {
        searchModel = new Zeal::SearchModel();
        tocModel = new Zeal::SearchModel();
    }

    ~TabState()
    {
        delete searchModel;
        delete tocModel;
    }

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

MainWindow::MainWindow(Core::Application *app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_application(app),
    m_settings(app->settings()),
    m_zealListModel(new ListModel(app->docsetRegistry(), this)),
    m_globalShortcut(new QxtGlobalShortcut(m_settings->showShortcut, this))
{
    ui->setupUi(this);

    connect(m_settings, &Core::Settings::updated, this, &MainWindow::applySettings);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

#ifdef USE_APPINDICATOR
    detectAppIndicatorSupport();
#endif

    if (m_settings->showSystrayIcon)
        createTrayIcon();

    // initialise key grabber
    connect(m_globalShortcut, &QxtGlobalShortcut::activated, this, &MainWindow::toggleWindow);

    setupTabBar();

    QShortcut *focusSearch = new QShortcut(QStringLiteral("Ctrl+K"), this);
    connect(focusSearch, &QShortcut::activated,
            ui->lineEdit, static_cast<void (SearchEdit::*)()>(&SearchEdit::setFocus));

    restoreGeometry(m_settings->windowGeometry);
    ui->splitter->restoreState(m_settings->verticalSplitterGeometry);

    // TODO: Custom headers and URL scheme for Qt WebEngine.
#ifndef USE_WEBENGINE
    ui->webView->page()->setNetworkAccessManager(m_application->networkManager());
#endif

    // Menu
    // File
    // Some platform plugins do not define QKeySequence::Quit.
    if (QKeySequence(QKeySequence::Quit).isEmpty())
        ui->actionQuit->setShortcut(QStringLiteral("Ctrl+Q"));
    else
        ui->actionQuit->setShortcut(QKeySequence::Quit);

    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Edit
    ui->actionFind->setShortcut(QKeySequence::Find);
    connect(ui->actionFind, &QAction::triggered, ui->webView, &SearchableWebView::showSearchBar);

    connect(ui->actionPreferences, &QAction::triggered, [this]() {
        m_globalShortcut->setEnabled(false);
        QScopedPointer<SettingsDialog> dialog(new SettingsDialog(m_application, this));
        dialog->exec();
        m_globalShortcut->setEnabled(true);
    });

    ui->actionBack->setShortcut(QKeySequence::Back);
    addAction(ui->actionBack);
    ui->actionForward->setShortcut(QKeySequence::Forward);
    addAction(ui->actionForward);
    connect(ui->actionBack, &QAction::triggered, ui->webView, &SearchableWebView::back);
    connect(ui->actionForward, &QAction::triggered, ui->webView, &SearchableWebView::forward);

    // Help Menu
    connect(ui->actionSubmitFeedback, &QAction::triggered, [this]() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/zealdocs/zeal/issues")));
    });
    connect(ui->actionCheckForUpdates, &QAction::triggered,
            m_application, &Core::Application::checkForUpdates);
    connect(ui->actionAboutZeal, &QAction::triggered, [this]() {
        QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
        dialog->exec();
    });
    connect(ui->actionAboutQt, &QAction::triggered, [this]() {
        QMessageBox::aboutQt(this);
    });

    // Update check
    connect(m_application, &Core::Application::updateCheckError, [this](const QString &message) {
        QMessageBox::warning(this, QStringLiteral("Zeal"), message);
    });

    connect(m_application, &Core::Application::updateCheckDone, [this](const QString &version) {
        if (version.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("Zeal"),
                                     tr("You are using the latest version."));
            return;
        }

        const int ret = QMessageBox::information(this, QStringLiteral("Zeal"),
                                                 QString(tr("Zeal <b>%1</b> is available. Open download page?")).arg(version),
                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://zealdocs.org/download.html")));
    });

    m_backMenu = new QMenu(ui->backButton);
    ui->backButton->setMenu(m_backMenu);

    m_forwardMenu = new QMenu(ui->forwardButton);
    ui->forwardButton->setMenu(m_forwardMenu);

    displayViewActions();

    // treeView and lineEdit
    ui->lineEdit->setTreeView(ui->treeView);
    ui->lineEdit->setFocus();
    setupSearchBoxCompletions();
    SearchItemDelegate *delegate = new SearchItemDelegate(ui->treeView);
    delegate->setDecorationRoles({Zeal::SearchModel::DocsetIconRole, Qt::DecorationRole});
    connect(ui->lineEdit, &QLineEdit::textChanged, [delegate](const QString &text) {
        delegate->setHighlight(Zeal::SearchQuery::fromString(text).query());
    });
    ui->treeView->setItemDelegate(delegate);

    ui->tocListView->setItemDelegate(new SearchItemDelegate(ui->tocListView));
    connect(ui->tocSplitter, &QSplitter::splitterMoved, this, [this]() {
        if (ui->tocListView->isVisible())
            m_settings->tocSplitterState = ui->tocSplitter->saveState();
    });

    createTab();

    connect(ui->treeView, &QTreeView::clicked, this, &MainWindow::openDocset);
    connect(ui->tocListView, &QListView::clicked, this, &MainWindow::openDocset);
    connect(ui->treeView, &QTreeView::activated, this, &MainWindow::openDocset);
    connect(ui->tocListView, &QListView::activated, this, &MainWindow::openDocset);
    connect(ui->forwardButton, &QPushButton::clicked, ui->webView, &SearchableWebView::forward);
    connect(ui->backButton, &QPushButton::clicked, ui->webView, &SearchableWebView::back);

    connect(ui->webView, &SearchableWebView::urlChanged, [this](const QUrl &url) {
        const QString name = docsetName(url);
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), docsetIcon(name));

        Docset *docset = m_application->docsetRegistry()->docset(name);
        if (docset)
            m_currentTabState->tocModel->setResults(docset->relatedLinks(url));

        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::titleChanged, [this](const QString &text) {
        m_tabBar->setTabText(m_tabBar->currentIndex(), text);
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::linkClicked, [this](const QUrl &url) {
        const QString message = tr("Do you want to open an external link?<br>URL: <b>%1</b>");
        int ret = QMessageBox::question(this, QStringLiteral("Zeal"), message.arg(url.toString()));
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::queryCompleted,
            this, [this](const QList<SearchResult> &results) {
        m_currentTabState->searchModel->setResults(results);
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::docsetRemoved,
            this, [this](const QString &name) {
        setupSearchBoxCompletions();
        for (TabState *tabState : m_tabStates) {
#ifdef USE_WEBENGINE
            if (docsetName(tabState->webPage->url()) != name)
                continue;

            tabState->webPage->load(QUrl(startPageUrl));
#else
            if (docsetName(tabState->webPage->mainFrame()->url()) != name)
                continue;

            tabState->tocModel->setResults();

            // optimization: disable updates temporarily because
            // removeSearchResultWithName can call {begin,end}RemoveRows
            // multiple times which can cause GUI updates to be suboptimal
            // in case of many rows to be removed
            ui->treeView->setUpdatesEnabled(false);
            tabState->searchModel->removeSearchResultWithName(name);
            ui->treeView->setUpdatesEnabled(true);

            tabState->webPage->mainFrame()->load(QUrl(startPageUrl));
#endif
            // TODO: Cleanup history
        }
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::docsetAdded,
            this, [this](const QString &) {
        setupSearchBoxCompletions();
    });

    connect(ui->lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        if (!m_currentTabState || text == m_currentTabState->searchQuery)
            return;

        m_currentTabState->searchQuery = text;
        m_application->docsetRegistry()->search(text);
        if (text.isEmpty()) {
            m_currentTabState->tocModel->setResults();
            displayTreeView();
        }
    });

    ui->actionNewTab->setShortcut(QKeySequence::AddTab);
    connect(ui->actionNewTab, &QAction::triggered, this, &MainWindow::createTab);
    addAction(ui->actionNewTab);

    // save the expanded items:
    connect(ui->treeView, &QTreeView::expanded, [this](QModelIndex index) {
        if (m_currentTabState->expansions.indexOf(index) == -1)
            m_currentTabState->expansions.append(index);
    });

    connect(ui->treeView, &QTreeView::collapsed, [this](QModelIndex index) {
        m_currentTabState->expansions.removeOne(index);
    });

#ifdef Q_OS_WIN32
    ui->actionCloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->actionCloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->actionCloseTab);
    connect(ui->actionCloseTab, &QAction::triggered, this, [this]() { closeTab(); });

    connect(ui->openUrlButton, &QPushButton::clicked, [this]() {
        const QUrl url(ui->webView->page()->history()->currentItem().url());
        if (url.scheme() != QLatin1String("qrc"))
            QDesktopServices::openUrl(url);
    });

    ui->actionNextTab->setShortcut(QKeySequence::NextChild);
    addAction(ui->actionNextTab);
    connect(ui->actionNextTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });

    // TODO: Use QKeySequence::PreviousChild, when QTBUG-15746 is fixed.
    ui->actionPreviousTab->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Tab);
    addAction(ui->actionPreviousTab);
    connect(ui->actionPreviousTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });

#ifdef Q_OS_OSX
    ui->treeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->tocListView->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    if (m_settings->checkForUpdate)
        m_application->checkForUpdates(true);
}

MainWindow::~MainWindow()
{
    m_settings->verticalSplitterGeometry = ui->splitter->saveState();
    m_settings->windowGeometry = saveGeometry();

    delete ui;
    qDeleteAll(m_tabStates);
}

void MainWindow::search(const SearchQuery &query)
{
    if (query.isEmpty())
        return;

    ui->lineEdit->setText(query.toString());
    ui->treeView->activated(ui->treeView->currentIndex());
}

void MainWindow::openDocset(const QModelIndex &index)
{
    const QVariant urlStr = index.sibling(index.row(), 1).data();
    if (urlStr.isNull())
        return;

    // TODO: Keep anchor separately from file address
    QStringList urlParts = urlStr.toString().split(QLatin1Char('#'));
    QUrl url = QUrl::fromLocalFile(urlParts[0]);
    if (urlParts.count() > 1)
        // NOTE: QUrl::DecodedMode is a fix for #121. Let's hope it doesn't break anything.
        url.setFragment(urlParts[1], QUrl::DecodedMode);

    ui->webView->load(url);

    // QWebEnginePage::load() always steals focus, so no need to do it twice.
#ifndef USE_WEBENGINE
    ui->webView->focus();
#endif
}

QString MainWindow::docsetName(const QUrl &url) const
{
    const QRegExp docsetRegex(QStringLiteral("/([^/]+)[.]docset"));
    return docsetRegex.indexIn(url.path()) != -1 ? docsetRegex.cap(1) : QString();
}

QIcon MainWindow::docsetIcon(const QString &docsetName) const
{
    Docset *docset = m_application->docsetRegistry()->docset(docsetName);
    return docset ? docset->icon() : QIcon(QStringLiteral(":/icons/logo/icon.png"));
}

void MainWindow::queryCompleted()
{
    displayTreeView();

    ui->treeView->setCurrentIndex(m_currentTabState->searchModel->index(0, 0, QModelIndex()));
    openDocset(ui->treeView->currentIndex());

    // Get focus back. QWebPageEngine::load() always steals focus.
    ui->lineEdit->setFocus(Qt::MouseFocusReason);
}

void MainWindow::selectTab(int index)
{
    if (index == -1)
        return;

    saveTabState();
    m_currentTabState = nullptr;
    reloadTabState();
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = m_tabBar->currentIndex();

    if (index == -1)
        return;

    // TODO: proper deletion here
    TabState *state = m_tabStates.takeAt(index);

    if (m_currentTabState == state)
        m_currentTabState = nullptr;

    delete state;

    m_tabBar->removeTab(index);

    if (m_tabStates.count() == 0)
        createTab();
}

void MainWindow::createTab()
{
    TabState *newTab = new TabState();

    connect(newTab->searchModel, &SearchModel::queryCompleted, this, &MainWindow::queryCompleted);
    connect(newTab->tocModel, &SearchModel::queryCompleted, this, &MainWindow::toggleToc);

    newTab->webPage = new QWebPage(ui->webView);
#ifdef USE_WEBENGINE
    newTab->webPage->load(QUrl(startPageUrl));
#else
    newTab->webPage->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    newTab->webPage->setNetworkAccessManager(m_application->networkManager());
    newTab->webPage->mainFrame()->load(QUrl(startPageUrl));
#endif

    m_tabStates.append(newTab);


    const int index = m_tabBar->addTab(QStringLiteral("title"));
    m_tabBar->setCurrentIndex(index);
}

void MainWindow::displayTreeView()
{
    TabState *tabState = currentTabState();

    if (!tabState->searchQuery.isEmpty()) {
        ui->treeView->setModel(tabState->searchModel);
        ui->treeView->setRootIsDecorated(false);
    } else {
        ui->treeView->setModel(m_zealListModel);
        ui->treeView->setColumnHidden(1, true);
        ui->treeView->setRootIsDecorated(true);
    }
    ui->treeView->reset();
}

void MainWindow::toggleToc()
{
    if (!currentTabState()->tocModel->isEmpty()) {
        ui->tocListView->show();
        ui->tocSplitter->restoreState(m_settings->tocSplitterState);
    } else {
        ui->tocListView->hide();
    }

}

TabState *MainWindow::currentTabState() const
{
    return m_tabStates.at(m_tabBar->currentIndex());
}

void MainWindow::saveTabState()
{
    if (!m_currentTabState)
        return;

    m_currentTabState->searchQuery = ui->lineEdit->text();
    m_currentTabState->selections = ui->treeView->selectionModel()->selectedIndexes();
    m_currentTabState->searchScrollPosition = ui->treeView->verticalScrollBar()->value();
    m_currentTabState->tocScrollPosition = ui->tocListView->verticalScrollBar()->value();
    m_currentTabState->webViewZoomFactor = ui->webView->zoomFactor();
}

void MainWindow::reloadTabState()
{
    TabState *tabState = currentTabState();

    ui->lineEdit->setText(tabState->searchQuery);
    ui->tocListView->setModel(tabState->tocModel);

    toggleToc();
    displayTreeView();

    // Bring back the selections and expansions
    ui->treeView->blockSignals(true);
    for (const QModelIndex &selection: tabState->selections)
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    for (const QModelIndex &expandedIndex: tabState->expansions)
        ui->treeView->expand(expandedIndex);
    ui->treeView->blockSignals(false);

    ui->webView->setPage(tabState->webPage);
    ui->webView->setZoomFactor(tabState->webViewZoomFactor);

    m_currentTabState = tabState;

    ui->treeView->verticalScrollBar()->setValue(m_currentTabState->searchScrollPosition);
    ui->tocListView->verticalScrollBar()->setValue(m_currentTabState->tocScrollPosition);

    displayViewActions();
}

// Sets up the search box autocompletions.
void MainWindow::setupSearchBoxCompletions()
{
    QStringList completions;
    for (const Docset * const docset: m_application->docsetRegistry()->docsets()) {
        if (docset->keywords().isEmpty())
            continue;

        completions << docset->keywords().first() + QLatin1Char(':');
    }

    ui->lineEdit->setCompletions(completions);
}

void MainWindow::setupTabBar()
{
    m_tabBar = new QTabBar(this);

    m_tabBar->installEventFilter(this);

    m_tabBar->setTabsClosable(true);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setDrawBase(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setStyleSheet(QStringLiteral("QTabBar::tab { width: 150px; }"));

    connect(m_tabBar, &QTabBar::currentChanged, this, &MainWindow::selectTab);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);

    for (int i = 1; i < 10; i++) {
        QAction *action = new QAction(m_tabBar);
#ifdef Q_OS_LINUX
        action->setShortcut(QStringLiteral("Alt+%1").arg(i));
#else
        action->setShortcut(QStringLiteral("Ctrl+%1").arg(i));
#endif
        if (i == 9) {
            connect(action, &QAction::triggered, [=]() {
                m_tabBar->setCurrentIndex(m_tabBar->count() - 1);
            });
        } else {
            connect(action, &QAction::triggered, [=]() {
                m_tabBar->setCurrentIndex(i - 1);
            });
        }

        addAction(action);
    }

    QHBoxLayout *layout = reinterpret_cast<QHBoxLayout *>(ui->navigationBar->layout());
    layout->insertWidget(2, m_tabBar, 0, Qt::AlignBottom);
}

void MainWindow::displayViewActions()
{
    ui->actionBack->setEnabled(ui->webView->canGoBack());
    ui->backButton->setEnabled(ui->webView->canGoBack());
    ui->actionForward->setEnabled(ui->webView->canGoForward());
    ui->forwardButton->setEnabled(ui->webView->canGoForward());

    ui->menuView->clear();
    ui->menuView->addAction(ui->actionBack);
    ui->menuView->addAction(ui->actionForward);
    ui->menuView->addSeparator();

    m_backMenu->clear();
    m_forwardMenu->clear();

    QWebHistory *history = ui->webView->page()->history();
    for (const QWebHistoryItem &item: history->backItems(10))
        m_backMenu->addAction(addHistoryAction(history, item));
    if (history->count() > 0)
        addHistoryAction(history, history->currentItem())->setEnabled(false);
    for (const QWebHistoryItem &item: history->forwardItems(10))
        m_forwardMenu->addAction(addHistoryAction(history, item));
}

QAction *MainWindow::addHistoryAction(QWebHistory *history, const QWebHistoryItem &item)
{
    const QIcon icon = docsetIcon(docsetName(item.url()));
    QAction *backAction = new QAction(icon, item.title(), ui->menuView);
    ui->menuView->addAction(backAction);
    connect(backAction, &QAction::triggered, [=](bool) {
        history->goToItem(item);
    });

    return backAction;
}

#ifdef USE_APPINDICATOR
void MainWindow::detectAppIndicatorSupport()
{
    const QByteArray xdgDesktop = qgetenv("XDG_CURRENT_DESKTOP");

    // Unity
    if (xdgDesktop == "Unity") {
        m_useAppIndicator = true;
        return;
    }

    // Cinnamon 2.8
    // Checking specifically for 2.8 because direct AppIndicator support will be dropped soon.
    if (xdgDesktop == "X-Cinnamon" && qgetenv("CINNAMON_VERSION").startsWith("2.8")) {
        m_useAppIndicator = true;
        return;
    }
}
#endif

#ifdef USE_APPINDICATOR
void appIndicatorToggleWindow(GtkMenu *menu, gpointer data)
{
    Q_UNUSED(menu);
    static_cast<MainWindow *>(data)->toggleWindow();
}
#endif

void MainWindow::createTrayIcon()
{
#ifdef USE_APPINDICATOR
    if (m_trayIcon || m_appIndicator)
        return;
#else
    if (m_trayIcon)
        return;
#endif

#ifdef USE_APPINDICATOR
    if (m_useAppIndicator) {
        m_appIndicatorMenu = gtk_menu_new();

        m_appIndicatorShowHideMenuItem = gtk_menu_item_new_with_label(qPrintable(tr("Hide")));
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorShowHideMenuItem);
        g_signal_connect(m_appIndicatorShowHideMenuItem, "activate",
                         G_CALLBACK(appIndicatorToggleWindow), this);

        m_appIndicatorMenuSeparator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorMenuSeparator);

        m_appIndicatorQuitMenuItem = gtk_menu_item_new_with_label(qPrintable(tr("Quit")));
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorQuitMenuItem);
        g_signal_connect(m_appIndicatorQuitMenuItem, "activate",
                         G_CALLBACK(QCoreApplication::quit), NULL);

        gtk_widget_show_all(m_appIndicatorMenu);

        // NOTE: Zeal icon has to be installed, otherwise app indicator won't be shown
        m_appIndicator = app_indicator_new("zeal", "zeal", APP_INDICATOR_CATEGORY_OTHER);

        app_indicator_set_status(m_appIndicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_menu(m_appIndicator, GTK_MENU(m_appIndicatorMenu));
    } else {  // others
#endif
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(windowIcon());
        m_trayIcon->setToolTip(QStringLiteral("Zeal"));

        connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
                return;

            toggleWindow();
        });

        QMenu *trayIconMenu = new QMenu(this);
        trayIconMenu->addAction(ui->actionQuit);

        m_trayIcon->setContextMenu(trayIconMenu);

        m_trayIcon->show();
#ifdef USE_APPINDICATOR
    }
#endif
}

void MainWindow::removeTrayIcon()
{
#ifdef USE_APPINDICATOR
    if (!m_trayIcon && !m_appIndicator)
        return;
#else
    if (!m_trayIcon)
        return;
#endif

#ifdef USE_APPINDICATOR
    if (m_useAppIndicator) {
        g_clear_object(&m_appIndicator);
        g_clear_object(&m_appIndicatorMenu);
        g_clear_object(&m_appIndicatorShowHideMenuItem);
        g_clear_object(&m_appIndicatorMenuSeparator);
        g_clear_object(&m_appIndicatorQuitMenuItem);
    } else {
#endif
        QMenu *trayIconMenu = m_trayIcon->contextMenu();
        delete m_trayIcon;
        m_trayIcon = nullptr;
        delete trayIconMenu;
#ifdef USE_APPINDICATOR
    }
#endif
}

void MainWindow::bringToFront()
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();

    ui->lineEdit->setFocus();
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
            QMouseEvent *e = reinterpret_cast<QMouseEvent *>(event);
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
        ui->lineEdit->setFocus();
        ui->lineEdit->clearQuery();
        break;
    case Qt::Key_Question:
        ui->lineEdit->setFocus();
        ui->lineEdit->selectQuery();
        break;
    default:
        QMainWindow::keyPressEvent(keyEvent);
        break;
    }
}

void MainWindow::applySettings()
{
    m_globalShortcut->setShortcut(m_settings->showShortcut);

    if (m_settings->showSystrayIcon)
        createTrayIcon();
    else
        removeTrayIcon();
}

void MainWindow::toggleWindow()
{
    const bool checkActive = sender() == m_globalShortcut;

    if (!isVisible() || (checkActive && !isActiveWindow())) {
#ifdef USE_APPINDICATOR
        if (m_appIndicator) {
            gtk_menu_item_set_label(GTK_MENU_ITEM(m_appIndicatorShowHideMenuItem),
                                    qPrintable(tr("Hide")));
        }
#endif
        bringToFront();
    } else {
#ifdef USE_APPINDICATOR
        if (m_trayIcon || m_appIndicator) {
            if (m_appIndicator) {
                gtk_menu_item_set_label(GTK_MENU_ITEM(m_appIndicatorShowHideMenuItem),
                                        qPrintable(tr("Show")));
            }
#else
        if (m_trayIcon) {
#endif
            hide();
        } else {
            showMinimized();
        }
    }
}
