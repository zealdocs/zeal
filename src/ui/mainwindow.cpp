#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "networkaccessmanager.h"
#include "searchitemdelegate.h"
#include "settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetregistry.h"
#include "registry/listmodel.h"
#include "registry/searchmodel.h"

#include <QAbstractEventDispatcher>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QTimer>

#ifdef USE_WEBENGINE
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#else
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#endif

#include <qxtglobalshortcut.h>

/// TODO: [Qt 5.5] Remove in favour of native Qt support (QTBUG-31762)
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

MainWindow::MainWindow(Core::Application *app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_application(app),
    m_settings(app->settings()),
    m_zealListModel(new ListModel(app->docsetRegistry(), this)),
    m_settingsDialog(new SettingsDialog(app, m_zealListModel, this)),
    m_globalShortcut(new QxtGlobalShortcut(m_settings->showShortcut, this))
{
    connect(m_settings, &Core::Settings::updated, this, &MainWindow::applySettings);

    m_tabBar = new QTabBar(this);
    m_tabBar->installEventFilter(this);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

    if (m_settings->showSystrayIcon)
        createTrayIcon();

    // initialise key grabber
    connect(m_globalShortcut, &QxtGlobalShortcut::activated, this, &MainWindow::toggleWindow);

    // initialise ui
    ui->setupUi(this);

    QShortcut *focusSearch = new QShortcut(QKeySequence(QStringLiteral("Ctrl+K")), this);
    focusSearch->setContext(Qt::ApplicationShortcut);
    connect(focusSearch, &QShortcut::activated,
            ui->lineEdit, static_cast<void (SearchEdit::*)()>(&SearchEdit::setFocus));

    restoreGeometry(m_settings->windowGeometry);
    ui->splitter->restoreState(m_settings->splitterGeometry);
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        m_settings->splitterGeometry = ui->splitter->saveState();
    });

    m_zealNetworkManager = new NetworkAccessManager(this);
#ifdef USE_WEBENGINE
    /// FIXME AngularJS workaround (zealnetworkaccessmanager.cpp)
#else
    ui->webView->page()->setNetworkAccessManager(m_zealNetworkManager);
#endif

    // menu
    if (QKeySequence(QKeySequence::Quit) != QKeySequence(QStringLiteral("Ctrl+Q"))) {
        ui->actionQuit->setShortcuts(QList<QKeySequence>{QKeySequence(QStringLiteral("Ctrl+Q")),
                                                         QKeySequence::Quit});
    } else {
        // Quit == Ctrl+Q - don't set the same sequence twice because it causes
        // "QAction::eventFilter: Ambiguous shortcut overload: Ctrl+Q"
        ui->actionQuit->setShortcuts(QList<QKeySequence>{QKeySequence::Quit});
    }
    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(ui->actionOptions, &QAction::triggered, [=]() {
        m_globalShortcut->setEnabled(false);
        m_settingsDialog->exec();
        m_globalShortcut->setEnabled(true);
    });

    ui->actionBack->setShortcut(QKeySequence::Back);
    addAction(ui->actionBack);
    ui->actionForward->setShortcut(QKeySequence::Forward);
    addAction(ui->actionForward);
    connect(ui->actionBack, &QAction::triggered, this, &MainWindow::back);
    connect(ui->actionForward, &QAction::triggered, this, &MainWindow::forward);

    // Help Menu
    connect(ui->actionReportProblem, &QAction::triggered, [this]() {
        QDesktopServices::openUrl(QStringLiteral("https://github.com/zealdocs/zeal/issues"));
    });
    connect(ui->actionCheckForUpdate, &QAction::triggered,
            m_application, &Core::Application::checkForUpdate);
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
            QMessageBox::information(this, QStringLiteral("Zeal"), tr("You are using the latest Zeal version."));
            return;
        }

        const int ret = QMessageBox::information(this, QStringLiteral("Zeal"),
                                                 QString(tr("A new version <b>%1</b> is available. Open download page?")).arg(version),
                                                 QMessageBox::Yes, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(QStringLiteral("http://zealdocs.org/download.html"));
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
    ui->treeView->setModel(m_zealListModel);
    ui->treeView->setColumnHidden(1, true);
    SearchItemDelegate *delegate = new SearchItemDelegate(ui->treeView);
    connect(ui->lineEdit, &QLineEdit::textChanged, [delegate](const QString &text) {
        delegate->setHighlight(Zeal::SearchQuery::fromString(text).query());
    });
    ui->treeView->setItemDelegate(delegate);

    createTab();
    /// FIXME: QTabBar does not emit currentChanged() after the first addTab() call
    reloadTabState();

    connect(ui->treeView, &QTreeView::clicked, [this](const QModelIndex &index) {
        m_treeViewClicked = true;
        ui->treeView->activated(index);
    });
    connect(ui->sections, &QListView::clicked, [this](const QModelIndex &index) {
        m_treeViewClicked = true;
        ui->sections->activated(index);
    });
    connect(ui->treeView, &QTreeView::activated, this, &MainWindow::openDocset);
    connect(ui->sections, &QListView::activated, this, &MainWindow::openDocset);
    connect(ui->forwardButton, &QPushButton::clicked, this, &MainWindow::forward);
    connect(ui->backButton, &QPushButton::clicked, this, &MainWindow::back);

    connect(ui->webView, &SearchableWebView::urlChanged, [this](const QUrl &url) {
        const QString name = docsetName(url);
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), docsetIcon(name));

        Docset *docset = m_application->docsetRegistry()->docset(name);
        if (docset)
            m_searchState->sectionsList->setResults(docset->relatedLinks(url));

        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::titleChanged, [this](const QString &) {
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::linkClicked, [this](const QUrl &url) {
        const QString message = tr("Do you want to open an external link?<br>URL: <b>%1</b>");
        int ret = QMessageBox::question(this, QStringLiteral("Zeal"), message.arg(url.toString()));
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
    });

    ui->sections->hide();
    ui->seeAlsoLabel->hide();
    ui->sections->setModel(m_searchState->sectionsList);
    connect(m_application->docsetRegistry(), &DocsetRegistry::queryCompleted, this, &MainWindow::onSearchComplete);

    connect(m_application->docsetRegistry(), &DocsetRegistry::docsetRemoved,
            this, [this](const QString &name) {
        for (SearchState *searchState : m_tabs) {
#ifdef USE_WEBENGINE
            if (docsetName(searchState->page->url()) != name)
                continue;

            searchState->page->load(QUrl(startPageUrl));
#else
            if (docsetName(searchState->page->mainFrame()->url()) != name)
                continue;

            searchState->page->mainFrame()->load(QUrl(startPageUrl));
#endif
            /// TODO: Cleanup history
        }
    });

    connect(ui->lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        if (!m_searchState || text == m_searchState->searchQuery)
            return;

        m_searchState->searchQuery = text;
        m_application->docsetRegistry()->search(text);
        if (text.isEmpty()) {
            m_searchState->sectionsList->setResults();
            ui->treeView->setModel(m_zealListModel);
            ui->treeView->setRootIsDecorated(true);
        }
    });

    ui->actionNewTab->setShortcut(QKeySequence::AddTab);
    connect(ui->actionNewTab, &QAction::triggered, this, &MainWindow::createTab);
    addAction(ui->actionNewTab);

    // save the expanded items:
    connect(ui->treeView, &QTreeView::expanded, [this](QModelIndex index) {
        if (m_searchState->expansions.indexOf(index) == -1)
            m_searchState->expansions.append(index);
    });

    connect(ui->treeView, &QTreeView::collapsed, [this](QModelIndex index) {
        m_searchState->expansions.removeOne(index);
    });

#ifdef Q_OS_WIN32
    ui->actionCloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->actionCloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->actionCloseTab);
    connect(ui->actionCloseTab, &QAction::triggered, this, &MainWindow::closeTab);

    m_tabBar->setTabsClosable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setDrawBase(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setStyleSheet(QStringLiteral("QTabBar::tab { width: 150px; }"));

    connect(m_tabBar, &QTabBar::currentChanged, this, &MainWindow::goToTab);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);

    ((QHBoxLayout *)ui->tabBarFrame->layout())->insertWidget(2, m_tabBar, 0, Qt::AlignBottom);

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

    ui->actionPreviousTab->setShortcut(QKeySequence::PreviousChild);
    addAction(ui->actionPreviousTab);
    connect(ui->actionPreviousTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });

#ifdef Q_OS_OSX
    ui->treeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->sections->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    /// TODO: Remove in the future releases
    // Check pre-0.1 docset path
    QString oldDocsetDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    oldDocsetDir.remove(QStringLiteral("Zeal/Zeal"));
    oldDocsetDir += QLatin1String("zeal/docsets");
    if (QFileInfo::exists(oldDocsetDir) && m_settings->docsetPath != oldDocsetDir) {
        QMessageBox::information(this, QStringLiteral("Zeal"),
                                 QString(tr("Old docset storage has been found in <b>%1</b>. "
                                            "You can move docsets to <b>%2</b> or change the docset storage path in the settings. <br><br>"
                                            "Please note, that old docsets cannot be updated automatically, so it is better to download your docsets again. <br><br>"
                                            "Remove or use the old docset storage to avoid this message in the future."))
                                 .arg(QDir::toNativeSeparators(oldDocsetDir), QDir::toNativeSeparators(m_settings->docsetPath)));
    }

    if (m_settings->checkForUpdate)
        m_application->checkForUpdate(true);
}

MainWindow::~MainWindow()
{
    delete ui;

    for (SearchState *state : m_tabs) {
        delete state->zealSearch;
        delete state->sectionsList;
        delete state;
    }
}

void MainWindow::openDocset(const QModelIndex &index)
{
    const QVariant urlStr = index.sibling(index.row(), 1).data();
    if (urlStr.isNull())
        return;

    /// TODO: Keep anchor separately from file address
    QStringList urlParts = urlStr.toString().split(QLatin1Char('#'));
    QUrl url = QUrl::fromLocalFile(urlParts[0]);
    if (urlParts.count() > 1)
        /// NOTE: QUrl::DecodedMode is a fix for #121. Let's hope it doesn't break anything.
        url.setFragment(urlParts[1], QUrl::DecodedMode);

    ui->webView->load(url);

    if (!m_treeViewClicked)
        ui->webView->focus();
    else
        m_treeViewClicked = false;
}

QString MainWindow::docsetName(const QUrl &url) const
{
    const QRegExp docsetRegex(QStringLiteral("/([^/]+)[.]docset"));
    return docsetRegex.indexIn(url.path()) != -1 ? docsetRegex.cap(1) : QString();
}

QIcon MainWindow::docsetIcon(const QString &docsetName) const
{
    const Docset * const docset = m_application->docsetRegistry()->docset(docsetName);
    if (!docset)
        return QIcon(QStringLiteral(":/icons/logo/icon.png"));
    return docset->icon();
}

void MainWindow::queryCompleted()
{
    m_treeViewClicked = true;

    ui->treeView->setModel(m_searchState->zealSearch);
    ui->treeView->setRootIsDecorated(false);
    ui->treeView->setCurrentIndex(m_searchState->zealSearch->index(0, 0, QModelIndex()));
    ui->treeView->activated(ui->treeView->currentIndex());
}

void MainWindow::goToTab(int index)
{
    if (index == -1)
        return;

    saveTabState();
    m_searchState = nullptr;
    reloadTabState();
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = m_tabBar->currentIndex();

    if (index == -1)
        return;

    /// TODO: proper deletion here
    SearchState *state = m_tabs.takeAt(index);

    if (m_searchState == state)
        m_searchState = nullptr;

    delete state->zealSearch;
    delete state->sectionsList;
    delete state;

    m_tabBar->removeTab(index);

    if (m_tabs.count() == 0)
        createTab();
}

void MainWindow::createTab()
{
    saveTabState();

    SearchState *newTab = new SearchState();
    newTab->zealSearch = new Zeal::SearchModel();
    newTab->sectionsList = new Zeal::SearchModel();

    connect(newTab->zealSearch, &SearchModel::queryCompleted, this, &MainWindow::queryCompleted);
    connect(newTab->sectionsList, &SearchModel::queryCompleted, [=]() {
        const bool hasResults = newTab->sectionsList->rowCount();
        ui->sections->setVisible(hasResults);
        ui->seeAlsoLabel->setVisible(hasResults);
    });

    newTab->page = new QWebPage(ui->webView);
#ifdef USE_WEBENGINE
    newTab->page->load(QUrl(startPageUrl));
#else
    newTab->page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    newTab->page->setNetworkAccessManager(m_zealNetworkManager);
    newTab->page->mainFrame()->load(QUrl(startPageUrl));
#endif

    m_tabs.append(newTab);


    const int index = m_tabBar->addTab(QStringLiteral("title"));
    m_tabBar->setCurrentIndex(index);
}

void MainWindow::displayTabs()
{
    ui->menuTabs->clear();
    ui->menuTabs->addAction(ui->actionNewTab);
    ui->menuTabs->addAction(ui->actionCloseTab);
    ui->menuTabs->addSeparator();
    ui->menuTabs->addAction(ui->actionNextTab);
    ui->menuTabs->addAction(ui->actionPreviousTab);
    ui->menuTabs->addSeparator();

    ui->actionNextTab->setEnabled(m_tabBar->count() > 1);
    ui->actionPreviousTab->setEnabled(m_tabBar->count() > 1);

    for (int i = 0; i < m_tabs.count(); i++) {
        SearchState *state = m_tabs.at(i);
#ifdef USE_WEBENGINE
        QString title = state->page->title();
#else
        QString title = state->page->history()->currentItem().title();
#endif
        QAction *action = ui->menuTabs->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == m_tabBar->currentIndex());

        if (i < 10) {
#ifdef Q_OS_LINUX
            const QKeySequence shortcut = QString("Alt+%1").arg(QString::number((i + 1) % 10));
#else
            const QKeySequence shortcut = QString("Ctrl+%1").arg(QString::number((i + 1) % 10));
#endif

            for (QAction *oldAction : actions()) {
                if (oldAction->shortcut() == shortcut)
                    removeAction(oldAction);
            }

            action->setShortcut(shortcut);
            addAction(action);
        }

        m_tabBar->setTabText(i, title);
        connect(action, &QAction::triggered, [=]() {
            m_tabBar->setCurrentIndex(i);
        });
    }
}

void MainWindow::reloadTabState()
{
    SearchState *searchState = m_tabs.at(m_tabBar->currentIndex());

    ui->lineEdit->setText(searchState->searchQuery);
    ui->sections->setModel(searchState->sectionsList);

    if (!searchState->searchQuery.isEmpty()) {
        ui->treeView->setModel(searchState->zealSearch);
        ui->treeView->setRootIsDecorated(false);
    } else {
        ui->treeView->setModel(m_zealListModel);
        ui->treeView->setRootIsDecorated(true);
        ui->treeView->reset();
    }

    // Bring back the selections and expansions
    ui->treeView->blockSignals(true);
    for (const QModelIndex &selection: searchState->selections)
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    for (const QModelIndex &expandedIndex: searchState->expansions)
        ui->treeView->expand(expandedIndex);
    ui->treeView->blockSignals(false);

    ui->webView->setPage(searchState->page);
    ui->webView->setZoomFactor(searchState->zoomFactor);

    int resultCount = searchState->sectionsList->rowCount();
    ui->sections->setVisible(resultCount > 1);
    ui->seeAlsoLabel->setVisible(resultCount > 1);

    m_searchState = searchState;

    // scroll after the object gets loaded
    /// TODO: [Qt 5.4] QTimer::singleShot(100, this, &MainWindow::scrollSearch);
    QTimer::singleShot(100, this, SLOT(scrollSearch()));

    displayViewActions();
}

void MainWindow::scrollSearch()
{
    ui->treeView->verticalScrollBar()->setValue(m_searchState->scrollPosition);
    ui->sections->verticalScrollBar()->setValue(m_searchState->sectionsScroll);
}

void MainWindow::saveTabState()
{
    if (!m_searchState)
        return;

    m_searchState->searchQuery = ui->lineEdit->text();
    m_searchState->selections = ui->treeView->selectionModel()->selectedIndexes();
    m_searchState->scrollPosition = ui->treeView->verticalScrollBar()->value();
    m_searchState->sectionsScroll = ui->sections->verticalScrollBar()->value();
    m_searchState->zoomFactor = ui->webView->zoomFactor();
}

void MainWindow::onSearchComplete()
{
    m_searchState->zealSearch->setResults(m_application->docsetRegistry()->queryResults());
}

// Sets up the search box autocompletions.
void MainWindow::setupSearchBoxCompletions()
{
    QStringList completions;
    for (const Docset * const docset: m_application->docsetRegistry()->docsets())
        completions << docset->keyword() + QLatin1Char(':');
    ui->lineEdit->setCompletions(completions);
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

    displayTabs();
}

void MainWindow::back()
{
    ui->webView->back();
    displayViewActions();
}

void MainWindow::forward()
{
    ui->webView->forward();
    displayViewActions();
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
    const QString desktop = getenv("XDG_CURRENT_DESKTOP");
    const bool isUnity = (desktop.toLower() == QLatin1String("unity"));

    if (isUnity) { // Application Indicators for Unity
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

        /// NOTE: Zeal icon has to be installed, otherwise app indicator won't be shown
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

            // Disable, when settings window is open
            if (m_settingsDialog->isVisible()) {
                m_settingsDialog->activateWindow();
                return;
            }

            toggleWindow();
        });

        QMenu *trayIconMenu = new QMenu(this);
        QAction *quitAction = trayIconMenu->addAction(tr("&Quit"));
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

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
    const QString desktop = getenv("XDG_CURRENT_DESKTOP");
    const bool isUnity = (desktop.toLower() == QLatin1String("unity"));

    if (isUnity) {
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

void MainWindow::bringToFront(const Zeal::SearchQuery &query)
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();

    if (!query.isEmpty()) {
        ui->lineEdit->setText(query.toString());
        ui->treeView->setFocus();
        ui->treeView->activated(ui->treeView->currentIndex());
    }
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
    m_settings->windowGeometry = saveGeometry();
    if (m_settings->showSystrayIcon && m_settings->hideOnClose) {
        event->ignore();
        toggleWindow();
    }
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_tabBar && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *e = reinterpret_cast<QMouseEvent *>(event);
        if (e->button() == Qt::MiddleButton) {
            const int index = m_tabBar->tabAt(e->pos());
            if (index >= 0) {
                closeTab(index);
                return true;
            }
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
