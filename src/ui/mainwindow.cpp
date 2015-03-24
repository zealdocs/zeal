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
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QTimer>

#ifdef USE_WEBENGINE
#include <QWebEngineSettings>
#else
#include <QWebFrame>
#include <QWebPage>
#endif

#include <qxtglobalshortcut.h>

#ifdef USE_LIBAPPINDICATOR
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
    m_tabBar = new QTabBar(this);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

    if (m_settings->showSystrayIcon)
        createTrayIcon();

    // initialise key grabber
    connect(m_globalShortcut, &QxtGlobalShortcut::activated, [this]() {
        if (!isVisible() || !isActiveWindow()) {
            bringToFront();
        } else {
#ifdef USE_LIBAPPINDICATOR
            if (m_trayIcon || m_indicator) {
#else
            if (m_trayIcon) {
#endif
                hide();
            } else {
                showMinimized();
            }
        }
    });

    m_application->docsetRegistry()->init(m_settings->docsetPath);

    // initialise ui
    ui->setupUi(this);

    setupShortcuts();

    restoreGeometry(m_settings->windowGeometry);
    ui->splitter->restoreState(m_settings->splitterGeometry);
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        m_settings->splitterGeometry = ui->splitter->saveState();
    });

    m_zealNetworkManager = new NetworkAccessManager();
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
    addAction(ui->actionQuit);
    connect(ui->actionQuit, &QAction::triggered, [=]() {
        m_settings->windowGeometry = saveGeometry();
    });
    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(ui->actionOptions, &QAction::triggered, [=]() {
        m_globalShortcut->setEnabled(false);

        if (m_settingsDialog->exec()) {
            m_globalShortcut->setShortcut(m_settings->showShortcut);

            if (m_settings->showSystrayIcon) {
                createTrayIcon();
            } else if (m_trayIcon) {
                QMenu *trayIconMenu = m_trayIcon->contextMenu();
                delete m_trayIcon;
                m_trayIcon = nullptr;
                delete trayIconMenu;
            }
        }

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
    connect(ui->actionAboutZeal, &QAction::triggered, [this]() {
        QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
        dialog->exec();
    });
    connect(ui->actionAboutQt, &QAction::triggered, [this]() {
        QMessageBox::aboutQt(this);
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
    ui->treeView->setItemDelegate(new SearchItemDelegate(ui->lineEdit, ui->treeView));

    createTab();

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
        loadSections(name, url);
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), docsetIcon(name));
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
            [this](const QString &name) {
        for (SearchState *searchState : m_tabs) {
            if (docsetName(searchState->page->mainFrame()->url()) != name)
                continue;

            searchState->page->mainFrame()->load(QUrl(startPageUrl));
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
        }
    });

    ui->actionNewTab->setShortcut(QKeySequence::AddTab);
    addAction(ui->actionNewTab);
    connect(ui->actionNewTab, &QAction::triggered, [this]() {
        saveTabState();
        createTab();
    });

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
}

MainWindow::~MainWindow()
{
    delete ui;
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
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setCurrentIndex(m_searchState->zealSearch->index(0, 0, QModelIndex()));
    ui->treeView->activated(ui->treeView->currentIndex());
}

void MainWindow::goToTab(int index)
{
    saveTabState();

    if (m_tabs.isEmpty())
        return;

    m_searchState = m_tabs.at(index);
    reloadTabState();
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = m_tabBar->currentIndex();

    /// TODO: proper deletion here
    SearchState *tab = m_tabs.takeAt(index);

    if (m_searchState == tab)
        m_searchState = nullptr;

    delete tab->zealSearch;
    delete tab->sectionsList;
    delete tab;

    m_tabBar->removeTab(index);

    if (m_tabs.count() == 0)
        createTab();
}

void MainWindow::createTab()
{
    SearchState *newTab = new SearchState();
    newTab->zealSearch = new Zeal::SearchModel();
    newTab->sectionsList = new Zeal::SearchModel();

    connect(newTab->zealSearch, &SearchModel::queryCompleted, this, &MainWindow::queryCompleted);
    connect(newTab->sectionsList, &SearchModel::queryCompleted, [=]() {
        const bool hasResults = newTab->sectionsList->rowCount(QModelIndex());
        ui->sections->setVisible(hasResults);
        ui->seeAlsoLabel->setVisible(hasResults);
    });

    ui->lineEdit->clear();

    newTab->page = new QWebPage(ui->webView);
#ifndef USE_WEBENGINE
    newTab->page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    newTab->page->setNetworkAccessManager(m_zealNetworkManager);
#endif

    ui->treeView->setModel(NULL);
    ui->treeView->setModel(m_zealListModel);
    ui->treeView->setColumnHidden(1, true);

    m_tabs.append(newTab);
    m_searchState = newTab;

    m_tabBar->addTab(QStringLiteral("title"));
    m_tabBar->setCurrentIndex(m_tabs.size() - 1);

    reloadTabState();
#ifdef USE_WEBENGINE
    newTab->page->load(QUrl(indexPageUrl));
#else
    newTab->page->mainFrame()->load(QUrl(startPageUrl));
#endif
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
            const QKeySequence shortcut = QString("Ctrl+%1").arg(QString::number((i + 1) % 10));

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
    ui->lineEdit->setText(m_searchState->searchQuery);
    ui->sections->setModel(m_searchState->sectionsList);

    if (!m_searchState->searchQuery.isEmpty()) {
        ui->treeView->setModel(m_searchState->zealSearch);
    } else {
        ui->treeView->setModel(m_zealListModel);
        ui->treeView->setColumnHidden(1, true);
    }

    // Bring back the selections and expansions.
    for (const QModelIndex &selection: m_searchState->selections)
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    for (const QModelIndex &expandedIndex: m_searchState->expansions)
        ui->treeView->expand(expandedIndex);

    ui->webView->setPage(m_searchState->page);
    ui->webView->setZealZoomFactor(m_searchState->zoomFactor);

    int resultCount = m_searchState->sectionsList->rowCount(QModelIndex());
    ui->sections->setVisible(resultCount > 1);
    ui->seeAlsoLabel->setVisible(resultCount > 1);

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
    m_searchState->zoomFactor = ui->webView->zealZoomFactor();
}

void MainWindow::onSearchComplete()
{
    m_searchState->zealSearch->setResults(m_application->docsetRegistry()->queryResults());
}

void MainWindow::loadSections(const QString &docsetName, const QUrl &url)
{
    Docset *docset = m_application->docsetRegistry()->docset(docsetName);
    if (!docset)
        return;

    m_searchState->sectionsList->setResults(docset->relatedLinks(url));
}

// Sets up the search box autocompletions.
void MainWindow::setupSearchBoxCompletions()
{
    QStringList completions;
    for (const Docset * const docset: m_application->docsetRegistry()->docsets())
        completions << docset->prefix + QLatin1Char(':');
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

QAction *MainWindow::addHistoryAction(QWebHistory *history, QWebHistoryItem item)
{
    const QIcon icon = docsetIcon(docsetName(item.url()));
    QAction *backAction = new QAction(icon, item.title(), ui->menuView);
    ui->menuView->addAction(backAction);
    connect(backAction, &QAction::triggered, [=](bool) {
        history->goToItem(item);
    });

    return backAction;
}

#ifdef USE_LIBAPPINDICATOR
void onQuit(GtkMenu *menu, gpointer data)
{
    Q_UNUSED(menu);
    QApplication *self = static_cast<QApplication *>(data);
    self->quit();
}

#endif

void MainWindow::createTrayIcon()
{
#ifdef USE_LIBAPPINDICATOR
    if (m_trayIcon || m_indicator) return;
#else
    if (m_trayIcon) return;
#endif

#ifdef USE_LIBAPPINDICATOR
    const QString desktop = getenv("XDG_CURRENT_DESKTOP");
    const bool isUnity = (desktop.toLower() == QLatin1String("unity"));

    if (isUnity) { // Application Indicators for Unity
        GtkWidget *menu;
        GtkWidget *quitItem;

        menu = gtk_menunew();

        quitItem = gtk_menuitem_new_with_label("Quit");
        gtk_menushell_append(GTK_menuSHELL(menu), quitItem);
        g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), qApp);
        gtk_widget_show(quitItem);

        m_indicator = app_indicator_new("zeal",
                                        icon.name().toLatin1().data(), APP_INDICATOR_CATEGORY_OTHER);

        app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_menu(m_indicator, GTK_MENU(menu));
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

            if (isVisible())
                hide();
            else
                bringToFront();
        });

        QMenu *trayIconMenu = new QMenu(this);
        QAction *quitAction = trayIconMenu->addAction(tr("&Quit"));
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        m_trayIcon->setContextMenu(trayIconMenu);

        m_trayIcon->show();
#ifdef USE_LIBAPPINDICATOR
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->windowGeometry = saveGeometry();
    if (m_settings->showSystrayIcon && m_settings->hideOnClose) {
        event->ignore();
        hide();
    }
}

void MainWindow::setupShortcuts()
{
    QShortcut *focusSearch = new QShortcut(QKeySequence(QStringLiteral("Ctrl+K")), this);
    focusSearch->setContext(Qt::ApplicationShortcut);
    connect(focusSearch, &QShortcut::activated, [=]() {
        ui->lineEdit->setFocus();
    });
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
