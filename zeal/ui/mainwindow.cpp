#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "zealdocsetsregistry.h"
#include "zeallistmodel.h"
#include "zealnativeeventfilter.h"
#include "zealnetworkaccessmanager.h"
#include "zealsearchitemdelegate.h"
#include "zealsearchquery.h"
#include "zealsettingsdialog.h"


#include <QAbstractEventDispatcher>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTimer>

#ifdef USE_WEBENGINE
    #include <QWebEngineSettings>
#else
    #include <QWebFrame>
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#ifdef USE_LIBAPPINDICATOR
#include <gtk/gtk.h>
#endif

#ifdef Q_OS_LINUX
#include "xcb_keysym.h"

#include <qpa/qplatformnativeinterface.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#endif

using namespace Zeal;

const QString serverName = "zeal_process_running";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_settings(new QSettings(this)),
    m_zealListModel(new ListModel(this)),
    m_settingsDialog(new SettingsDialog(m_zealListModel, this)),
    m_nativeFilter(new NativeEventFilter(this))
{
    // server for detecting already running instances
    m_localServer = new QLocalServer(this);
    connect(m_localServer, &QLocalServer::newConnection, [&]() {
        QLocalSocket *connection = m_localServer->nextPendingConnection();
        // Wait a little while the other side writes the bytes
        connection->waitForReadyRead();
        QString indata = connection->readAll();
        if (!indata.isEmpty())
            bringToFrontAndSearch(indata);
    });
    QLocalServer::removeServer(serverName);  // remove in case previous instance crashed
    m_localServer->listen(serverName);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

    if (m_settings->value("hidingBehavior", "systray").toString() == "systray")
        createTrayIcon();

    QKeySequence keySequence
            = m_settings->value(QStringLiteral("hotkey"), QStringLiteral("Alt+Space")).value<QKeySequence>();

    // initialise key grabber
    connect(m_nativeFilter, &NativeEventFilter::hotKeyPressed, [&]() {
        if (!isVisible() || !isActiveWindow()) {
            bringToFront(true);
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
    qApp->eventDispatcher()->installNativeEventFilter(m_nativeFilter);
    setHotKey(keySequence);

    // initialise docsets
    DocsetsRegistry::instance()->initialiseDocsets();

    // initialise ui
    ui->setupUi(this);

    setupShortcuts();

    restoreGeometry(m_settings->value("geometry").toByteArray());
    ui->splitter->restoreState(m_settings->value("splitter").toByteArray());
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        m_settings->setValue("splitter", ui->splitter->saveState());
    });

    applyWebPageStyle();
    m_zealNetworkManager = new NetworkAccessManager();
    m_zealNetworkManager->setProxy(m_settingsDialog->httpProxy());
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
        m_settings->setValue("geometry", saveGeometry());
    });
    connect(ui->action_Quit, SIGNAL(triggered()), qApp, SLOT(quit()));

    connect(m_settingsDialog, SIGNAL(refreshRequested()), this, SLOT(refreshRequest()));
    connect(m_settingsDialog, SIGNAL(minFontSizeChanged(int)), this, SLOT(changeMinFontSize(int)));
    connect(m_settingsDialog, SIGNAL(webPageStyleUpdated()), this, SLOT(applyWebPageStyle()));

    connect(ui->action_Options, &QAction::triggered, [=]() {
        m_settingsDialog->setHotKey(m_hotKey);
        m_nativeFilter->setEnabled(false);
        if (m_settingsDialog->exec()) {
            setHotKey(m_settingsDialog->hotKey());
            if (m_settings->value("hidingBehavior").toString() == "systray") {
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
                                                 m_settings->value("minFontSize").toInt());
        }
        m_nativeFilter->setEnabled(true);
        ui->treeView->reset();
    });

    ui->action_Back->setShortcut(QKeySequence::Back);
    addAction(ui->action_Back);
    ui->action_Forward->setShortcut(QKeySequence::Forward);
    addAction(ui->action_Forward);
    connect(ui->action_Back, &QAction::triggered, this, &MainWindow::back);
    connect(ui->action_Forward, &QAction::triggered, this, &MainWindow::forward);

    connect(ui->action_About, &QAction::triggered, [&]() {
        QMessageBox::about(this, "About Zeal",
                           QString("This is Zeal ") + ZEAL_VERSION
                           + " - a documentation browser.\n\n"
                           + "For details see http://zealdocs.org/");
    });
    connect(ui->action_About_QT, &QAction::triggered, [&]() {
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
    ui->treeView->setItemDelegate(new ZealSearchItemDelegate(ui->treeView, ui->lineEdit,
                                                             ui->treeView));
    m_treeViewClicked = false;

    createTab();

    connect(ui->treeView, &QTreeView::clicked, [&](const QModelIndex &index) {
        m_treeViewClicked = true;
        ui->treeView->activated(index);
    });
    connect(ui->sections, &QListView::clicked, [&](const QModelIndex &index) {
        m_treeViewClicked = true;
        ui->sections->activated(index);
    });
    connect(ui->treeView, &QTreeView::activated, this, &MainWindow::openDocset);
    connect(ui->sections, &QListView::activated, this, &MainWindow::openDocset);
    connect(ui->forwardButton, &QPushButton::clicked, this, &MainWindow::forward);
    connect(ui->backButton, &QPushButton::clicked, this, &MainWindow::back);

    connect(ui->webView, &SearchableWebView::urlChanged, [&](const QUrl &url) {
        const QString name = docsetName(url);
        if (DocsetsRegistry::instance()->names().contains(name))
            loadSections(name, url);

        m_tabBar.setTabIcon(m_tabBar.currentIndex(), docsetIcon(name));
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::titleChanged, [&](const QString &) {
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::linkClicked, [&](const QUrl &url) {
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
    connect(ui->lineEdit, &QLineEdit::textChanged, [&](const QString &text) {
        if (text == m_searchState->searchQuery)
            return;

        m_searchState->searchQuery = text;
        m_searchState->zealSearch.setQuery(text);
        if (text.isEmpty())
            ui->treeView->setModel(m_zealListModel);
    });

    ui->action_NewTab->setShortcut(QKeySequence::AddTab);
    addAction(ui->action_NewTab);
    connect(ui->action_NewTab, &QAction::triggered, [&]() {
        saveTabState();
        createTab();
    });

    // save the expanded items:
    connect(ui->treeView, &QTreeView::expanded, [&](QModelIndex index) {
        if (m_searchState->expansions.indexOf(index) == -1)
            m_searchState->expansions.append(index);
    });

    connect(ui->treeView, &QTreeView::collapsed, [&](QModelIndex index) {
        m_searchState->expansions.removeOne(index);
    });

#ifdef Q_OS_WIN32
    ui->action_CloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->action_CloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->action_CloseTab);
    connect(ui->action_CloseTab, &QAction::triggered, [&]() {
        closeTab();
    });

    m_tabBar.setTabsClosable(true);
    m_tabBar.setExpanding(false);
    m_tabBar.setUsesScrollButtons(true);
    m_tabBar.setDrawBase(false);

    connect(&m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    ((QHBoxLayout *)ui->frame_2->layout())->insertWidget(2, &m_tabBar, 0, Qt::AlignBottom);

    connect(&m_tabBar, &QTabBar::currentChanged, this, &MainWindow::goToTab);

    connect(ui->openUrlButton, &QPushButton::clicked, [&]() {
        QUrl url(ui->webView->page()->history()->currentItem().url());
        if (url.scheme() != "qrc")
            QDesktopServices::openUrl(url);
    });

    ui->action_NextTab->setShortcut(QKeySequence::NextChild);
    addAction(ui->action_NextTab);
    connect(ui->action_NextTab, &QAction::triggered, [&]() {
        m_tabBar.setCurrentIndex((m_tabBar.currentIndex() + 1) % m_tabBar.count());
    });

    ui->action_PreviousTab->setShortcut(QKeySequence::PreviousChild);
    addAction(ui->action_PreviousTab);
    connect(ui->action_PreviousTab, &QAction::triggered, [&]() {
        m_tabBar.setCurrentIndex((m_tabBar.currentIndex() - 1 + m_tabBar.count()) % m_tabBar.count());
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
    QRegExp docsetRegex(QStringLiteral("/([^/]+)[.]docset"));
    return docsetRegex.indexIn(url.path()) != -1 ? docsetRegex.cap(1) : QStringLiteral("");
}

QIcon MainWindow::docsetIcon(const QString &docsetName) const
{
    if (DocsetsRegistry::instance()->names().contains(docsetName))
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
        index = m_tabBar.currentIndex();

    // TODO: proper deletion here
    m_tabs.removeAt(index);

    if (m_tabs.count() == 0)
        createTab();
    m_tabBar.removeTab(index);
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

    ui->lineEdit->setText("");

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

    m_tabBar.addTab("title");
    m_tabBar.setCurrentIndex(m_tabs.size() - 1);

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

    ui->action_NextTab->setEnabled(m_tabBar.count() > 1);
    ui->action_PreviousTab->setEnabled(m_tabBar.count() > 1);

    for (int i = 0; i < m_tabs.count(); i++) {
        SearchState *state = m_tabs.at(i);
#ifdef USE_WEBENGINE
        QString title = state->page->title();
#else
        QString title = state->page->history()->currentItem().title();
#endif
        QAction *action = ui->menu_Tabs->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == m_tabBar.currentIndex());
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
        m_tabBar.setTabText(i, title);
        connect(action, &QAction::triggered, [=]() {
            m_tabBar.setCurrentIndex(i);
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
    QString dir = DocsetsRegistry::instance()->dir(docsetName).absolutePath();
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
    for (DocsetsRegistry::DocsetEntry docset: DocsetsRegistry::instance()->docsets())
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

        connect(m_trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
            if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
                return;

            if (isVisible())
                hide();
            else
                bringToFront(false);
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

void MainWindow::bringToFront(bool withHack)
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();

#ifndef Q_OS_WIN32
    // Very ugly workaround for the problem described at http://stackoverflow.com/questions/14553810/
    // (just show and hide a modal dialog box, which for some reason restores proper keyboard focus)
    if (withHack) {
        m_hackDialog.setGeometry(0, 0, 0, 0);
        m_hackDialog.setModal(true);
        m_hackDialog.show();
        QTimer::singleShot(100, &m_hackDialog, SLOT(reject()));
    }
#else
    Q_UNUSED(withHack)
#endif
}

void MainWindow::bringToFrontAndSearch(const QString &query)
{
    bringToFront(true);
    m_searchState->zealSearch.setQuery(query);
    ui->lineEdit->setText(query);
    ui->treeView->setFocus();
    ui->treeView->activated(ui->treeView->currentIndex());
}

bool MainWindow::startHidden()
{
    return m_settings->value("startupBehavior", "window").toString() == "systray";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("geometry", saveGeometry());
    event->ignore();
    hide();
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
        break;
    }
}

void MainWindow::setHotKey(const QKeySequence &hotKey_)
{
    // platform-specific code for global key grabbing
#ifdef Q_OS_WIN32
    UINT i_vk, i_mod = 0;
    if (!m_hotKey.isEmpty()) {
        // disable previous hotkey
        UnregisterHotKey(NULL, 10);
    }
    m_hotKey = hotKey_;
    m_nativeFilter->setHotKey(m_hotKey);
    m_settings->setValue("hotkey", m_hotKey);
    if (m_hotKey.isEmpty())
        return;
    int key = m_hotKey[0];
    if (key & Qt::ALT) i_mod |= MOD_ALT;
    if (key & Qt::CTRL) i_mod |= MOD_CONTROL;
    if (key & Qt::SHIFT) i_mod |= MOD_SHIFT;
    key = key & ~(Qt::ALT | Qt::CTRL | Qt::SHIFT | Qt::META);
#ifndef VK_VOLUME_DOWN
#define VK_VOLUME_DOWN          0xAE
#define VK_VOLUME_UP            0xAF
#endif

#ifndef VK_MEDIA_NEXT_TRACK
#define VK_MEDIA_NEXT_TRACK     0xB0
#define VK_MEDIA_PREV_TRACK     0xB1
#define VK_MEDIA_STOP           0xB2
#define VK_MEDIA_PLAY_PAUSE     0xB3
#endif

#ifndef VK_PAGEUP
#define VK_PAGEUP               0x21
#define VK_PAGEDOWN             0x22
#endif
    switch (key) {
    case Qt::Key_Left:
        i_vk = VK_LEFT;
        break;
    case Qt::Key_Right:
        i_vk = VK_RIGHT;
        break;
    case Qt::Key_Up:
        i_vk = VK_UP;
        break;
    case Qt::Key_Down:
        i_vk = VK_DOWN;
        break;
    case Qt::Key_Space:
        i_vk = VK_SPACE;
        break;
    case Qt::Key_Escape:
        i_vk = VK_ESCAPE;
        break;
    case Qt::Key_Enter:
        i_vk = VK_RETURN;
        break;
    case Qt::Key_Return:
        i_vk = VK_RETURN;
        break;
    case Qt::Key_F1:
        i_vk = VK_F1;
        break;
    case Qt::Key_F2:
        i_vk = VK_F2;
        break;
    case Qt::Key_F3:
        i_vk = VK_F3;
        break;
    case Qt::Key_F4:
        i_vk = VK_F4;
        break;
    case Qt::Key_F5:
        i_vk = VK_F5;
        break;
    case Qt::Key_F6:
        i_vk = VK_F6;
        break;
    case Qt::Key_F7:
        i_vk = VK_F7;
        break;
    case Qt::Key_F8:
        i_vk = VK_F8;
        break;
    case Qt::Key_F9:
        i_vk = VK_F9;
        break;
    case Qt::Key_F10:
        i_vk = VK_F10;
        break;
    case Qt::Key_F11:
        i_vk = VK_F11;
        break;
    case Qt::Key_F12:
        i_vk = VK_F12;
        break;
    case Qt::Key_PageUp:
        i_vk = VK_PAGEUP;
        break;
    case Qt::Key_PageDown:
        i_vk = VK_PAGEDOWN;
        break;
    case Qt::Key_Home:
        i_vk = VK_HOME;
        break;
    case Qt::Key_End:
        i_vk = VK_END;
        break;
    case Qt::Key_Insert:
        i_vk = VK_INSERT;
        break;
    case Qt::Key_Delete:
        i_vk = VK_DELETE;
        break;
    case Qt::Key_VolumeDown:
        i_vk = VK_VOLUME_DOWN;
        break;
    case Qt::Key_VolumeUp:
        i_vk = VK_VOLUME_UP;
        break;
    case Qt::Key_MediaTogglePlayPause:
        i_vk = VK_MEDIA_PLAY_PAUSE;
        break;
    case Qt::Key_MediaStop:
        i_vk = VK_MEDIA_STOP;
        break;
    case Qt::Key_MediaPrevious:
        i_vk = VK_MEDIA_PREV_TRACK;
        break;
    case Qt::Key_MediaNext:
        i_vk = VK_MEDIA_NEXT_TRACK;
        break;
    default:
        i_vk = toupper(key);
        break;
    }

    if (!RegisterHotKey(NULL, 10, i_mod, i_vk)) {
        m_hotKey = QKeySequence();
        m_nativeFilter->setHotKey(m_hotKey);
        m_settings->setValue("hotkey", m_hotKey);
        QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
    }
#endif // Q_OS_WIN32

#ifdef Q_OS_LINUX
    auto platform = qApp->platformNativeInterface();

    xcb_connection_t *c
        = static_cast<xcb_connection_t *>(platform->nativeResourceForWindow("connection", 0));
    xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(c);

    if (!m_hotKey.isEmpty()) {
        // remove previous bindings from all screens
        xcb_screen_iterator_t iter;
        iter = xcb_setup_roots_iterator(xcb_get_setup(c));
        for (; iter.rem; xcb_screen_next(&iter))
            xcb_ungrab_key(c, XCB_GRAB_ANY, iter.data->root, XCB_MOD_MASK_ANY);
    }
    m_hotKey = hotKey_;
    m_nativeFilter->setHotKey(m_hotKey);
    m_settings->setValue("hotkey", m_hotKey);

    if (m_hotKey.isEmpty()) return;

    xcb_keysym_t keysym = GetX11Key(m_hotKey[0]);
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, keysym), keycode;

    if (!keycodes) {
        m_hotKey = QKeySequence();
        m_nativeFilter->setHotKey(m_hotKey);
        m_settings->setValue("hotkey", m_hotKey);
        QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
        free(keysyms);
        return;
    }

    // add bindings for all screens
    xcb_screen_iterator_t iter;
    iter = xcb_setup_roots_iterator(xcb_get_setup(c));
    bool any_failed = false;
    for (; iter.rem; xcb_screen_next(&iter)) {
        int i = 0;
        while (keycodes[i] != XCB_NO_SYMBOL) {
            keycode = keycodes[i];
            for (auto modifier : GetX11Modifier(c, keysyms, m_hotKey[0])) {
                auto cookie = xcb_grab_key_checked(c, true, iter.data->root,
                                                   modifier, keycode, XCB_GRAB_MODE_SYNC,
                                                   XCB_GRAB_MODE_SYNC);
                if (xcb_request_check(c, cookie))
                    any_failed = true;
            }
            i += 1;
        }
    }
    if (any_failed) {
        QMessageBox::warning(this, "Key binding warning",
                             "Warning: Global hotkey binding problem detected. Some other program might have a conflicting key binding with "
                             "<strong>" + m_hotKey.toString() + "</strong>"
                                                              ". If the hotkey doesn't work, try closing some programs or using a different hotkey.");
    }
    free(keysyms);
    free(keycodes);
#endif // Q_OS_LINUX
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
    if (m_settings->contains("minFontSize")) {
        int minFont = m_settings->value("minFontSize").toInt();
        QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, minFont);
    }
}
