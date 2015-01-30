#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "networkaccessmanager.h"
#include "searchitemdelegate.h"
#include "settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetsregistry.h"
#include "registry/listmodel.h"
#include "registry/searchquery.h"

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
#endif

#include <QxtGlobalShortcut>

#ifdef USE_LIBAPPINDICATOR
#include <gtk/gtk.h>
#endif

using namespace Zeal;

MainWindow::MainWindow(Core::Application *app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_application(app),
    m_settings(app->settings()),
    m_zealListModel(new ListModel(this)),
    m_settingsDialog(new SettingsDialog(app, m_zealListModel, this)),
    m_globalShortcut(new QxtGlobalShortcut(this))
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

    DocsetsRegistry::instance()->initialiseDocsets(m_settings->docsetPath);

    // initialise ui
    ui->setupUi(this);

    setupShortcuts();

    restoreGeometry(m_settings->windowGeometry);
    ui->splitter->restoreState(m_settings->splitterGeometry);
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        m_settings->splitterGeometry = ui->splitter->saveState();
    });

    applyWebPageStyle();
    m_zealNetworkManager = new NetworkAccessManager();
#ifdef USE_WEBENGINE
    // FIXME AngularJS workaround (zealnetworkaccessmanager.cpp)
#else
    ui->webView->page()->setNetworkAccessManager(m_zealNetworkManager);
#endif

    // menu
    if (QKeySequence(QKeySequence::Quit) != QKeySequence("Ctrl+Q")) {
        ui->action_Quit->setShortcuts(QList<QKeySequence>{QKeySequence(
                                                              "Ctrl+Q"), QKeySequence::Quit});
    } else {
        // Quit == Ctrl+Q - don't set the same sequence twice because it causes
        // "QAction::eventFilter: Ambiguous shortcut overload: Ctrl+Q"
        ui->action_Quit->setShortcuts(QList<QKeySequence>{QKeySequence::Quit});
    }
    addAction(ui->action_Quit);
    connect(ui->action_Quit, &QAction::triggered, [=]() {
        m_settings->windowGeometry = saveGeometry();
    });
    connect(ui->action_Quit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(m_settingsDialog, &SettingsDialog::refreshRequested,
            this, &MainWindow::refreshRequest);
    connect(m_settingsDialog, &SettingsDialog::minFontSizeChanged,
            this, &MainWindow::changeMinFontSize);
    connect(m_settingsDialog, &SettingsDialog::webPageStyleUpdated,
            this, &MainWindow::applyWebPageStyle);

    connect(ui->action_Options, &QAction::triggered, [=]() {
        m_globalShortcut->setEnabled(false);

        if (m_settingsDialog->exec()) {
            m_settings->save();
            m_globalShortcut->setShortcut(m_settings->showShortcut);

            if (m_settings->showSystrayIcon) {
                createTrayIcon();
            } else if (m_trayIcon) {
                QMenu *trayIconMenu = m_trayIcon->contextMenu();
                delete m_trayIcon;
                m_trayIcon = nullptr;
                delete trayIconMenu;
            }
        } else {
            // cancelled - restore previous value
            ui->webView->settings()->setFontSize(QWebSettings::MinimumFontSize,
                                                 m_settings->minimumFontSize);
        }

        ui->treeView->reset();
        m_globalShortcut->setEnabled(true);
    });

    ui->action_Back->setShortcut(QKeySequence::Back);
    addAction(ui->action_Back);
    ui->action_Forward->setShortcut(QKeySequence::Forward);
    addAction(ui->action_Forward);
    connect(ui->action_Back, &QAction::triggered, this, &MainWindow::back);
    connect(ui->action_Forward, &QAction::triggered, this, &MainWindow::forward);

    connect(ui->action_About, &QAction::triggered, [this]() {
        QMessageBox::about(this, QStringLiteral("About Zeal"),
                           QStringLiteral("This is Zeal ") +
                           QStringLiteral(ZEAL_VERSION) +
                           QStringLiteral(" - a documentation browser.\n\n") +
                           QStringLiteral("For details see http://zealdocs.org/"));
    });
    connect(ui->action_About_QT, &QAction::triggered, [this]() {
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
    m_treeViewClicked = false;

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
        if (DocsetsRegistry::instance()->contains(name))
            loadSections(name, url);

        m_tabBar->setTabIcon(m_tabBar->currentIndex(), docsetIcon(name));
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::titleChanged, [this](const QString &) {
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::linkClicked, [](const QUrl &url) {
        QMessageBox question;
        QString messageFormat = QString("Do you want to go to an external link?\nUrl: %1");
        question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        question.setText(messageFormat.arg(url.toString()));
        if (question.exec() == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
    });

    ui->sections->hide();
    ui->sections_lab->hide();
    ui->sections->setModel(&m_searchState->sectionsList);
    connect(DocsetsRegistry::instance(), &DocsetsRegistry::queryCompleted, this, &MainWindow::onSearchComplete);
    connect(ui->lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        if (text == m_searchState->searchQuery)
            return;

        m_searchState->searchQuery = text;
        m_searchState->zealSearch.setQuery(text);
        if (text.isEmpty())
            ui->treeView->setModel(m_zealListModel);
    });

    ui->action_NewTab->setShortcut(QKeySequence::AddTab);
    addAction(ui->action_NewTab);
    connect(ui->action_NewTab, &QAction::triggered, [this]() {
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
    ui->action_CloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->action_CloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->action_CloseTab);
    connect(ui->action_CloseTab, &QAction::triggered, this, &MainWindow::closeTab);

    m_tabBar->setTabsClosable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setDrawBase(false);

    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    ((QHBoxLayout *)ui->frame_2->layout())->insertWidget(2, m_tabBar, 0, Qt::AlignBottom);

    connect(m_tabBar, &QTabBar::currentChanged, this, &MainWindow::goToTab);

    connect(ui->openUrlButton, &QPushButton::clicked, [this]() {
        QUrl url(ui->webView->page()->history()->currentItem().url());
        if (url.scheme() != "qrc")
            QDesktopServices::openUrl(url);
    });

    ui->action_NextTab->setShortcut(QKeySequence::NextChild);
    addAction(ui->action_NextTab);
    connect(ui->action_NextTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });

    ui->action_PreviousTab->setShortcut(QKeySequence::PreviousChild);
    addAction(ui->action_PreviousTab);
    connect(ui->action_PreviousTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openDocset(const QModelIndex &index)
{
    if (!index.sibling(index.row(), 1).data().isNull()) {
        QStringList url_l = index.sibling(index.row(), 1).data().toString().split('#');
        QUrl url = QUrl::fromLocalFile(url_l[0]);
        if (url_l.count() > 1)
            url.setFragment(url_l[1]);
        ui->webView->load(url);

        if (!m_treeViewClicked)
            ui->webView->focus();
        else
            m_treeViewClicked = false;
    }
}

QString MainWindow::docsetName(const QUrl &url) const
{
    const QRegExp docsetRegex(QStringLiteral("/([^/]+)[.]docset"));
    return docsetRegex.indexIn(url.path()) != -1 ? docsetRegex.cap(1) : QString();
}

QIcon MainWindow::docsetIcon(const QString &docsetName) const
{
    if (DocsetsRegistry::instance()->contains(docsetName))
        return DocsetsRegistry::instance()->icon(docsetName).pixmap(32, 32);
    else
        return QIcon();
}

void MainWindow::queryCompleted()
{
    m_treeViewClicked = true;

    ui->treeView->setModel(&m_searchState->zealSearch);
    ui->treeView->reset();
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setCurrentIndex(m_searchState->zealSearch.index(0, 0, QModelIndex()));
    ui->treeView->activated(ui->treeView->currentIndex());
}

void MainWindow::goToTab(int index)
{
    saveTabState();
    m_searchState = m_tabs.at(index);
    reloadTabState();
}

void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = m_tabBar->currentIndex();

    // TODO: proper deletion here
    m_tabs.removeAt(index);

    if (m_tabs.count() == 0)
        createTab();
    m_tabBar->removeTab(index);
}

void MainWindow::createTab()
{
    SearchState *newTab = new SearchState();
    connect(&newTab->zealSearch, &SearchModel::queryCompleted, this,
            &MainWindow::queryCompleted);
    connect(&newTab->sectionsList, &SearchModel::queryCompleted, [=]() {
        int resultCount = newTab->sectionsList.rowCount(QModelIndex());
        ui->sections->setVisible(resultCount > 1);
        ui->sections_lab->setVisible(resultCount > 1);
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

    m_tabBar->addTab("title");
    m_tabBar->setCurrentIndex(m_tabs.size() - 1);

    reloadTabState();
#ifdef USE_WEBENGINE
    newTab->page->load(QUrl("qrc:///webpage/Welcome.html"));
#else
    newTab->page->mainFrame()->load(QUrl("qrc:///webpage/Welcome.html"));
#endif
}

void MainWindow::displayTabs()
{
    ui->menu_Tabs->clear();
    ui->menu_Tabs->addAction(ui->action_NewTab);
    ui->menu_Tabs->addAction(ui->action_CloseTab);
    ui->menu_Tabs->addSeparator();
    ui->menu_Tabs->addAction(ui->action_NextTab);
    ui->menu_Tabs->addAction(ui->action_PreviousTab);
    ui->menu_Tabs->addSeparator();

    ui->action_NextTab->setEnabled(m_tabBar->count() > 1);
    ui->action_PreviousTab->setEnabled(m_tabBar->count() > 1);

    for (int i = 0; i < m_tabs.count(); i++) {
        SearchState *state = m_tabs.at(i);
#ifdef USE_WEBENGINE
        QString title = state->page->title();
#else
        QString title = state->page->history()->currentItem().title();
#endif
        QAction *action = ui->menu_Tabs->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == m_tabBar->currentIndex());
        if (i < 10) {
            QString shortcut;
            if (i == 9)
                shortcut = QString("Ctrl+%1").arg(QString::number(0));
            else
                shortcut = QString("Ctrl+%1").arg(QString::number(i+1));
            auto actions_ = actions();
            for (int i = 0; i < actions_.length(); ++i) {
                if (actions_[i]->shortcut().toString() == shortcut)
                    removeAction(actions_[i]);
            }
            addAction(action);
            action->setShortcut(QKeySequence(shortcut));
        }

        if (title.length() >= 20) {
            title.truncate(17);
            title += "...";
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
    ui->sections->setModel(&m_searchState->sectionsList);
    ui->treeView->reset();

    if (!m_searchState->searchQuery.isEmpty()) {
        ui->treeView->setModel(&m_searchState->zealSearch);
    } else {
        ui->treeView->setModel(m_zealListModel);
        ui->treeView->setColumnHidden(1, true);
    }

    // Bring back the selections and expansions.
    for (QModelIndex selection: m_searchState->selections)
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    for (QModelIndex expandedIndex: m_searchState->expansions)
        ui->treeView->expand(expandedIndex);

    ui->webView->setPage(m_searchState->page);
    ui->webView->setZealZoomFactor(m_searchState->zoomFactor);

    int resultCount = m_searchState->sectionsList.rowCount(QModelIndex());
    ui->sections->setVisible(resultCount > 1);
    ui->sections_lab->setVisible(resultCount > 1);

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
    m_searchState->searchQuery = ui->lineEdit->text();
    m_searchState->selections = ui->treeView->selectionModel()->selectedIndexes();
    m_searchState->scrollPosition = ui->treeView->verticalScrollBar()->value();
    m_searchState->sectionsScroll = ui->sections->verticalScrollBar()->value();
    m_searchState->zoomFactor = ui->webView->zealZoomFactor();
}

void MainWindow::onSearchComplete()
{
    m_searchState->zealSearch.onQueryCompleted(DocsetsRegistry::instance()->queryResults());
}

void MainWindow::loadSections(const QString &docsetName, const QUrl &url)
{
    QString dir = DocsetsRegistry::instance()->entry(docsetName).documentPath;
    QString urlPath = url.path();
    int dirPosition = urlPath.indexOf(dir);
    QString path = url.path().mid(dirPosition + dir.size() + 1);
    // resolve the url to use the docset related path.
    QList<SearchResult> results = DocsetsRegistry::instance()->relatedLinks(docsetName, path);
    m_searchState->sectionsList.onQueryCompleted(results);
}

// Sets up the search box autocompletions.
void MainWindow::setupSearchBoxCompletions()
{
    QStringList completions;
    for (const Docset &docset: DocsetsRegistry::instance()->docsets())
        completions << QString("%1:").arg(docset.prefix);
    ui->lineEdit->setCompletions(completions);
}

void MainWindow::displayViewActions()
{
    ui->action_Back->setEnabled(ui->webView->canGoBack());
    ui->backButton->setEnabled(ui->webView->canGoBack());
    ui->action_Forward->setEnabled(ui->webView->canGoForward());
    ui->forwardButton->setEnabled(ui->webView->canGoForward());

    ui->menu_View->clear();
    ui->menu_View->addAction(ui->action_Back);
    ui->menu_View->addAction(ui->action_Forward);
    ui->menu_View->addSeparator();

    m_backMenu->clear();
    m_forwardMenu->clear();

    QWebHistory *history = ui->webView->page()->history();
    for (QWebHistoryItem item: history->backItems(10))
        m_backMenu->addAction(addHistoryAction(history, item));
    if (history->count() > 0)
        addHistoryAction(history, history->currentItem())->setEnabled(false);
    for (QWebHistoryItem item: history->forwardItems(10))
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
    QAction *backAction = new QAction(icon, item.title(), ui->menu_View);
    ui->menu_View->addAction(backAction);
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
    const bool isUnity = (desktop.toLower() == "unity");

    if (isUnity) { // Application Indicators for Unity
        GtkWidget *menu;
        GtkWidget *quitItem;

        menu = gtk_menu_new();

        quitItem = gtk_menu_item_new_with_label("Quit");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quitItem);
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

            if (isVisible())
                hide();
            else
                bringToFront();
        });

        QMenu *trayIconMenu = new QMenu(this);
        QAction *quitAction = trayIconMenu->addAction(QStringLiteral("&Quit"));
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        m_trayIcon->setContextMenu(trayIconMenu);

        m_trayIcon->show();
#ifdef USE_LIBAPPINDICATOR
    }
#endif
}

void MainWindow::bringToFront(const QString &query)
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();

    if (!query.isEmpty()) {
        m_searchState->zealSearch.setQuery(query);
        ui->lineEdit->setText(query);
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
    QShortcut *focusSearch = new QShortcut(QKeySequence("Ctrl+K"), this);
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

void MainWindow::refreshRequest()
{
    ui->treeView->reset();
}

void MainWindow::changeMinFontSize(int minFont)
{
    QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, minFont);
}

void MainWindow::applyWebPageStyle()
{
    QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize,
                                                m_settings->minimumFontSize);
}
