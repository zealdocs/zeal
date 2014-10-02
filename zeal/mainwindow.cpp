#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include "zealsearchquery.h"
#include "zealdocsetsregistry.h"
#include "zealsearchitemdelegate.h"
#include "zealnetworkaccessmanager.h"

#include <QtDebug>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QAbstractEventDispatcher>
#include <QMessageBox>
#include <QStyleFactory>
#include <QLocalSocket>
#include <QLayout>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QWebSettings>
#include <QNetworkProxyFactory>
#include <QAbstractNetworkCache>
#include <QWebFrame>
#include <QWebHistory>
#include <QScrollBar>
#include <QShortcut>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_LIBAPPINDICATOR
#include <gtk/gtk.h>
#endif

#ifdef LINUX
#include <qpa/qplatformnativeinterface.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include "xcb_keysym.h"
#endif

const QString serverName = "zeal_process_running";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings("Zeal", "Zeal"),
    settingsDialog(zealList)
{
    trayIcon = nullptr;
#ifdef USE_LIBAPPINDICATOR
    indicator = nullptr;
#endif
    trayIconMenu = nullptr;

    // server for detecting already running instances
    localServer = new QLocalServer(this);
    connect(localServer, &QLocalServer::newConnection, [&]() {
        QLocalSocket *connection = localServer->nextPendingConnection();
        // Wait a little while the other side writes the bytes
        connection->waitForReadyRead();
        QString indata = connection->readAll();
        if(!indata.isEmpty()) {
            bringToFrontAndSearch(indata);
        }
    });
    QLocalServer::removeServer(serverName);  // remove in case previous instance crashed
    localServer->listen(serverName);

#if (!defined(WIN32) && !defined(OSX))
    // Default style sometimes (when =windows) doesn't work well with Linux
    qApp->setStyle(QStyleFactory::create("fusion"));
#endif

    // initialise icons
#if (defined(OSX) || defined(WIN32))
    icon = QIcon(":/zeal.ico");
#else
    QIcon::setThemeName("hicolor");
    icon = QIcon::fromTheme("zeal");
#endif
    setWindowIcon(icon);
    if(settings.value("hidingBehavior", "systray").toString() == "systray")
        createTrayIcon();

    QKeySequence keySequence;
    if(settings.value("hotkey").isNull()) {
        keySequence = QKeySequence("Alt+Space");
    } else {
        keySequence = settings.value("hotkey").value<QKeySequence>();
    }

    // initialise key grabber
    connect(&nativeFilter, &ZealNativeEventFilter::gotHotKey, [&]() {
        if(!isVisible() || !isActiveWindow()) {
            bringToFront(true);
        } else {
#ifdef USE_LIBAPPINDICATOR
            if(trayIcon || indicator) {
#else
            if(trayIcon) {
#endif
                hide();
            } else {
                showMinimized();
            }
        }
    });
    qApp->eventDispatcher()->installNativeEventFilter(&nativeFilter);
    setHotKey(keySequence);

    // initialise docsets
    docsets->initialiseDocsets();

    // initialise ui
    ui->setupUi(this);

    setupShortcuts();

    restoreGeometry(settings.value("geometry").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        settings.setValue("splitter", ui->splitter->saveState());
    });
    ui->webView->settings()->setFontSize(QWebSettings::MinimumFontSize, settings.value("minFontSize").toInt());
    ZealNetworkAccessManager * zealNaManager = new ZealNetworkAccessManager();
    zealNaManager->setProxy(settingsDialog.httpProxy());
    ui->webView->page()->setNetworkAccessManager(zealNaManager);

    
    // menu
    if (QKeySequence(QKeySequence::Quit) != QKeySequence("Ctrl+Q")) {
        ui->action_Quit->setShortcuts(QList<QKeySequence>{QKeySequence("Ctrl+Q"), QKeySequence::Quit});
    } else {
        // Quit == Ctrl+Q - don't set the same sequence twice because it causes
        // "QAction::eventFilter: Ambiguous shortcut overload: Ctrl+Q"
        ui->action_Quit->setShortcuts(QList<QKeySequence>{QKeySequence::Quit});
    }
    addAction(ui->action_Quit);
    connect(ui->action_Quit, &QAction::triggered, [=]() { settings.setValue("geometry", saveGeometry()); });
    connect(ui->action_Quit, SIGNAL(triggered()), qApp, SLOT(quit()));

    connect(&settingsDialog, SIGNAL(refreshRequested()), this, SLOT(refreshRequest()));
    connect(&settingsDialog, SIGNAL(minFontSizeChanged(int)), this, SLOT(changeMinFontSize(int)));

    connect(ui->action_Options, &QAction::triggered, [=]() {
        settingsDialog.setHotKey(hotKey);
        nativeFilter.setEnabled(false);
        if(settingsDialog.exec()) {
            setHotKey(settingsDialog.hotKey());
            if(settings.value("hidingBehavior").toString() == "systray") {
                createTrayIcon();
            } else if(trayIcon) {
                trayIcon->deleteLater();
                trayIconMenu->deleteLater();
                trayIcon = nullptr;
                trayIconMenu = nullptr;
            }
        } else {
            // cancelled - restore previous value
            ui->webView->settings()->setFontSize(QWebSettings::MinimumFontSize, settings.value("minFontSize").toInt());
        }
        nativeFilter.setEnabled(true);
        ui->treeView->reset();
    });

    ui->action_Back->setShortcut(QKeySequence::Back);
    addAction(ui->action_Back);
    ui->action_Forward->setShortcut(QKeySequence::Forward);
    addAction(ui->action_Forward);
    connect(ui->action_Back, &QAction::triggered, this, &MainWindow::back);
    connect(ui->action_Forward, &QAction::triggered, this, &MainWindow::forward);

    connect(ui->action_About, &QAction::triggered,
            [&]() { QMessageBox::about(this, "About Zeal",
                QString("This is Zeal ") + ZEAL_VERSION + " - a documentation browser.\n\n"
                "For details see http://zealdocs.org/"); });
    connect(ui->action_About_QT, &QAction::triggered,
            [&]() { QMessageBox::aboutQt(this); });
    displayViewActions();

    // treeView and lineEdit
    ui->lineEdit->setTreeView(ui->treeView);
    ui->lineEdit->setFocus();
    setupSearchBoxCompletions();
    ui->treeView->setModel(&zealList);
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setItemDelegate(new ZealSearchItemDelegate(ui->treeView, ui->lineEdit, ui->treeView));
#if QT_VERSION < QT_VERSION_CHECK(5, 1, 0) && defined(WIN32)
    // overriding subElementRect doesn't work with Qt 5.0.0, but is required to display
    // selected item frame correctly in Windows (for patch see https://codereview.qt-project.org/#change,46559)
    // This is a workaround for Qt < 5.1 - selecting whole rows leads to always rendering the frame.
    // (Only the frame is larger than the list item, which is different from default behaviour.)
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
#endif
    treeViewClicked = false;

    createTab();

    connect(ui->treeView, &QTreeView::clicked, [&](const QModelIndex& index) {
        treeViewClicked = true;
        ui->treeView->activated(index);
    });
    connect(ui->sections, &QListView::clicked, [&](const QModelIndex &index) {
        treeViewClicked = true;
        ui->sections->activated(index);
    });
    connect(ui->treeView, &QTreeView::activated, this, &MainWindow::openDocset);
    connect(ui->sections, &QListView::activated, this, &MainWindow::openDocset);
    connect(ui->forwardButton, &QPushButton::clicked, this, &MainWindow::forward);
    connect(ui->backButton, &QPushButton::clicked, this, &MainWindow::back);

    connect(ui->webView, &SearchableWebView::urlChanged, [&](const QUrl &url) {
        QString urlPath = url.path();
        QString docsetName = getDocsetName(urlPath);
        QIcon icon = docsetIcon(docsetName);
        if (docsets->names().contains(docsetName)) {
            loadSections(docsetName, url);
        }

        tabBar.setTabIcon(tabBar.currentIndex(), icon);
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
        if (question.exec() == QMessageBox::Yes) {
            QDesktopServices::openUrl(url);
        }
    });

    ui->sections->hide();
    ui->sections_lab->hide();
    ui->sections->setModel(&searchState->sectionsList);
    connect(docsets, &ZealDocsetsRegistry::queryCompleted, this, &MainWindow::onSearchComplete);
    connect(ui->lineEdit, &QLineEdit::textChanged, [&](const QString& text) {
        if (text == searchState->searchQuery) {
            return;
        }

        searchState->searchQuery = text;
        searchState->zealSearch.setQuery(text);
        if(text.isEmpty()) {
            ui->treeView->setModel(&zealList);
        }
    });

    ui->backButton->setMenu(&backMenu);
    ui->forwardButton->setMenu(&forwardMenu);

    ui->action_NewTab->setShortcut(QKeySequence::AddTab);
    addAction(ui->action_NewTab);
    connect(ui->action_NewTab, &QAction::triggered, [&]() {
        saveTabState();
        createTab();
    });

    // save the expanded items:
    connect(ui->treeView, &QTreeView::expanded, [&](QModelIndex index) {
        if (searchState->expansions.indexOf(index) == -1) {
            searchState->expansions.append(index);
        }
    });

    connect(ui->treeView, &QTreeView::collapsed, [&](QModelIndex index) {
        searchState->expansions.removeOne(index);
    });

#ifdef WIN32
    ui->action_CloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->action_CloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->action_CloseTab);
    connect(ui->action_CloseTab, &QAction::triggered, [&]() {
        closeTab(-1);
    });

    tabBar.setTabsClosable(false);
    tabBar.setExpanding(false);
    tabBar.setUsesScrollButtons(true);
    tabBar.setDrawBase(false);
    tabBar.setStyle(QStyleFactory::create("fusion"));

    connect(&tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    ((QHBoxLayout*)ui->frame_2->layout())->insertWidget(2, &tabBar, 0, Qt::AlignBottom);

    connect(&tabBar, &QTabBar::currentChanged, this, &MainWindow::goToTab);

    connect(ui->openUrlButton, &QPushButton::clicked, [&]() {
        QUrl url(ui->webView->page()->history()->currentItem().url());
        if (url.scheme() != "qrc") {
            QDesktopServices::openUrl(url);
        }
    });

    ui->action_NextTab->setShortcut(QKeySequence::NextChild);
    addAction(ui->action_NextTab);
    connect(ui->action_NextTab, &QAction::triggered, [&]() {
        tabBar.setCurrentIndex((tabBar.currentIndex() + 1) % tabBar.count());
    });

    ui->action_PreviousTab->setShortcut(QKeySequence::PreviousChild);
    addAction(ui->action_PreviousTab);
    connect(ui->action_PreviousTab, &QAction::triggered, [&]() {
        tabBar.setCurrentIndex((tabBar.currentIndex() - 1 + tabBar.count()) % tabBar.count());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete localServer;
}

void MainWindow::openDocset(const QModelIndex &index)
{
    if(!index.sibling(index.row(), 1).data().isNull()) {
        QStringList url_l = index.sibling(index.row(), 1).data().toString().split('#');
        QUrl url = QUrl::fromLocalFile(url_l[0]);
        if(url_l.count() > 1) {
            url.setFragment(url_l[1]);
        }
        ui->webView->load(url);

        if (!treeViewClicked)
            ui->webView->focus();
        else
            treeViewClicked = false;
    }
}

QIcon MainWindow::docsetIcon(QString docsetName)
{
    if (docsets->names().contains(docsetName)) {
        return docsets->icon(docsetName).pixmap(32,32);
    } else {
        return QIcon();
    }
}

void MainWindow::queryCompleted()
{
    treeViewClicked = true;

    ui->treeView->setModel(&searchState->zealSearch);
    ui->treeView->reset();
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setCurrentIndex(searchState->zealSearch.index(0, 0, QModelIndex()));
    ui->treeView->activated(ui->treeView->currentIndex());
}

void MainWindow::goToTab(int index)
{
    saveTabState();
    searchState = tabs.at(index);
    reloadTabState();
}

void MainWindow::closeTab(int index = -1)
{
    if (index == -1) {
        index = tabBar.currentIndex();
    }

    // TODO: proper deletion here
    tabs.removeAt(index);

    if (tabs.count() == 0) {
        createTab();
    }
    tabBar.removeTab(index);
}

void MainWindow::createTab()
{
    SearchState *newTab = new SearchState();
    connect(&newTab->zealSearch, &ZealSearchModel::queryCompleted, this, &MainWindow::queryCompleted);
    connect(&newTab->sectionsList, &ZealSearchModel::queryCompleted, [=]() {
        int resultCount = newTab->sectionsList.rowCount(QModelIndex());
        ui->sections->setVisible(resultCount > 1);
        ui->sections_lab->setVisible(resultCount > 1);
    });

    ui->lineEdit->setText("");

    newTab->page = new QWebPage(ui->webView);
    newTab->page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

    ui->treeView->setModel(NULL);
    ui->treeView->setModel(&zealList);
    ui->treeView->setColumnHidden(1, true);

    tabs.append(newTab);
    searchState = newTab;

    tabBar.addTab("title");
    tabBar.setCurrentIndex(tabs.size() - 1);

    reloadTabState();
    newTab->page->mainFrame()->load(QUrl("qrc:///webpage/Welcome.html"));
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

    ui->action_NextTab->setEnabled(tabBar.count() > 1);
    ui->action_PreviousTab->setEnabled(tabBar.count() > 1);

    for (int i = 0; i < tabs.count(); i++) {
        SearchState *state = tabs.at(i);
        QString title = state->page->history()->currentItem().title();
        QAction *action = ui->menu_Tabs->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == tabBar.currentIndex());
        if (i < 10) {
            QString shortcut = QString("Ctrl+%1").arg(QString::number(i+1));
            auto actions_ = actions();
            for (int i = 0; i < actions_.length(); ++i) {
                 if (actions_[i]->shortcut().toString() == shortcut) {
                       removeAction(actions_[i]);
                 }
            }
            addAction(action);
            action->setShortcut(QKeySequence(shortcut));
        }

        if (title.length() >= 20) {
            title.truncate(17);
            title += "...";
        }
        tabBar.setTabText(i, title);
        connect(action, &QAction::triggered, [=]() {
            tabBar.setCurrentIndex(i);
        });
    }
}

void MainWindow::reloadTabState()
{
    ui->lineEdit->setText(searchState->searchQuery);
    ui->sections->setModel(&searchState->sectionsList);
    ui->treeView->reset();

    if (!searchState->searchQuery.isEmpty()) {
        ui->treeView->setModel(&searchState->zealSearch);
    } else {
        ui->treeView->setModel(&zealList);
        ui->treeView->setColumnHidden(1, true);
    }

    // Bring back the selections and expansions.
    for (QModelIndex selection: searchState->selections) {
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    }
    for (QModelIndex expandedIndex: searchState->expansions) {
        ui->treeView->expand(expandedIndex);
    }

    ui->webView->setPage(searchState->page);

    int resultCount = searchState->sectionsList.rowCount(QModelIndex());
    ui->sections->setVisible(resultCount > 1);
    ui->sections_lab->setVisible(resultCount > 1);

    // scroll after the object gets loaded
    QTimer::singleShot(100, this, SLOT(scrollSearch()));

    displayViewActions();
}

void MainWindow::scrollSearch()
{
    ui->treeView->verticalScrollBar()->setValue(searchState->scrollPosition);
    ui->sections->verticalScrollBar()->setValue(searchState->sectionsScroll);
}

void MainWindow::saveTabState()
{
    searchState->searchQuery = ui->lineEdit->text();
    searchState->selections = ui->treeView->selectionModel()->selectedIndexes();
    searchState->scrollPosition = ui->treeView->verticalScrollBar()->value();
    searchState->sectionsScroll = ui->sections->verticalScrollBar()->value();
}

void MainWindow::onSearchComplete()
{
    searchState->zealSearch.onQueryCompleted(docsets->getQueryResults());
}

void MainWindow::loadSections(const QString docsetName, const QUrl &url)
{
    QString dir = docsets->dir(docsetName).absolutePath();
    QString urlPath = url.path();
    int dirPosition = urlPath.indexOf(dir);
    QString path = url.path().mid(dirPosition + dir.size() + 1);
    // resolve the url to use the docset related path.
    QList<ZealSearchResult> results = docsets->getRelatedLinks(docsetName, path);
    searchState->sectionsList.onQueryCompleted(results);
}

// Sets up the search box autocompletions.
void MainWindow::setupSearchBoxCompletions() {
    QStringList completions;
    for (ZealDocsetsRegistry::docsetEntry docset: docsets->docsets()) {
        completions << QString("%1:").arg(docset.prefix);
    }
    ui->lineEdit->setCompletions(completions);
}

QString MainWindow::getDocsetName(QString urlPath) {
    QRegExp docsetRegex("/([^/]+)[.]docset");
    return (docsetRegex.indexIn(urlPath) != -1)
            ? docsetRegex.cap(1)
            : "";
}

void MainWindow::displayViewActions() {
    ui->action_Back->setEnabled(ui->webView->canGoBack());
    ui->backButton->setEnabled(ui->webView->canGoBack());
    ui->action_Forward->setEnabled(ui->webView->canGoForward());
    ui->forwardButton->setEnabled(ui->webView->canGoForward());

    ui->menu_View->clear();
    ui->menu_View->addAction(ui->action_Back);
    ui->menu_View->addAction(ui->action_Forward);
    ui->menu_View->addSeparator();

    backMenu.clear();
    forwardMenu.clear();

    QWebHistory *history = ui->webView->page()->history();
    for (QWebHistoryItem item: history->backItems(10)) {
        backMenu.addAction(addHistoryAction(history, item));
    }
    addHistoryAction(history, history->currentItem())->setEnabled(false);
    for (QWebHistoryItem item: history->forwardItems(10)) {
        forwardMenu.addAction(addHistoryAction(history, item));
    }

    displayTabs();
}

void MainWindow::back() {
    ui->webView->back();
    displayViewActions();
}

void MainWindow::forward() {
    ui->webView->forward();
    displayViewActions();
}

QAction *MainWindow::addHistoryAction(QWebHistory *history, QWebHistoryItem item)
{
    QString docsetName = getDocsetName(item.url().toString());
    QIcon icon = docsetIcon(docsetName);

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
    if(trayIcon || indicator) return;
#else
    if(trayIcon) return;
#endif

    QString desktop;
    bool isUnity;

    desktop = getenv("XDG_CURRENT_DESKTOP");
    isUnity = (desktop.toLower() == "unity");

#ifdef USE_LIBAPPINDICATOR
    if(isUnity) //Application Indicators for Unity
    {
        GtkWidget *menu;
        GtkWidget *quitItem;

        menu = gtk_menu_new();

        quitItem = gtk_menu_item_new_with_label("Quit");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quitItem);
        g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), qApp);
        gtk_widget_show(quitItem);

        indicator = app_indicator_new("zeal", icon.name().toLatin1().data(), APP_INDICATOR_CATEGORY_OTHER);

        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_menu(indicator, GTK_MENU(menu));
    } else {  //others
#endif
        trayIconMenu = new QMenu(this);
        auto quitAction = trayIconMenu->addAction("&Quit");
        connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setIcon(icon);
        trayIcon->setToolTip("Zeal");
        connect(trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
            if(reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if(isVisible()) hide();
                else bringToFront(false);
            }
        });
        trayIcon->show();
#ifdef USE_LIBAPPINDICATOR
    }
#endif
}

void MainWindow::bringToFront(bool withHack)
{
    show();
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();

#ifndef WIN32
    // Very ugly workaround for the problem described at http://stackoverflow.com/questions/14553810/
    // (just show and hide a modal dialog box, which for some reason restores proper keyboard focus)
    if(withHack) {
        hackDialog.setGeometry(0, 0, 0, 0);
        hackDialog.setModal(true);
        hackDialog.show();
        QTimer::singleShot(100, &hackDialog, SLOT(reject()));
    }
#endif
}

void MainWindow::bringToFrontAndSearch(const QString query)
{
    bringToFront(true);
    searchState->zealSearch.setQuery(query);
    ui->lineEdit->setText(query);
    ui->treeView->setFocus();
    ui->treeView->activated(ui->treeView->currentIndex());
}

bool MainWindow::startHidden()
{
    if(settings.value("startupBehavior", "window").toString() == "systray")
        return true;
    return false;
}

void MainWindow::setupShortcuts()
{
    QShortcut* focusSearch = new QShortcut(QKeySequence("Ctrl+K"), this);
    focusSearch->setContext(Qt::ApplicationShortcut);
    connect(focusSearch, &QShortcut::activated, [=]() {
        ui->lineEdit->setFocus();
    });
}

// Captures global events in order to pass them to the search bar.
void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        ui->lineEdit->setFocus();
        ui->lineEdit->clearQuery();
    } else if (keyEvent->key() == Qt::Key_Question) {
        ui->lineEdit->setFocus();
        ui->lineEdit->selectQuery();
    }
}

void MainWindow::setHotKey(const QKeySequence& hotKey_) {
    // platform-specific code for global key grabbing
#ifdef WIN32
    UINT i_vk, i_mod = 0;
    if(!hotKey.isEmpty()) {
        // disable previous hotkey
        UnregisterHotKey(NULL, 10);
    }
    hotKey = hotKey_;
    nativeFilter.setHotKey(hotKey);
    settings.setValue("hotkey", hotKey);
    if(hotKey.isEmpty()) return;
    int key = hotKey[0];
    if(key & Qt::ALT) i_mod |= MOD_ALT;
    if(key & Qt::CTRL) i_mod |= MOD_CONTROL;
    if(key & Qt::SHIFT) i_mod |= MOD_SHIFT;
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
    switch(key) {
        case Qt::Key_Left: i_vk = VK_LEFT; break;
        case Qt::Key_Right: i_vk = VK_RIGHT; break;
        case Qt::Key_Up: i_vk = VK_UP; break;
        case Qt::Key_Down: i_vk = VK_DOWN; break;
        case Qt::Key_Space: i_vk = VK_SPACE; break;
        case Qt::Key_Escape: i_vk = VK_ESCAPE; break;
        case Qt::Key_Enter: i_vk = VK_RETURN; break;
        case Qt::Key_Return: i_vk = VK_RETURN; break;
        case Qt::Key_F1: i_vk = VK_F1; break;
        case Qt::Key_F2: i_vk = VK_F2; break;
        case Qt::Key_F3: i_vk = VK_F3; break;
        case Qt::Key_F4: i_vk = VK_F4; break;
        case Qt::Key_F5: i_vk = VK_F5; break;
        case Qt::Key_F6: i_vk = VK_F6; break;
        case Qt::Key_F7: i_vk = VK_F7; break;
        case Qt::Key_F8: i_vk = VK_F8; break;
        case Qt::Key_F9: i_vk = VK_F9; break;
        case Qt::Key_F10: i_vk = VK_F10; break;
        case Qt::Key_F11: i_vk = VK_F11; break;
        case Qt::Key_F12: i_vk = VK_F12; break;
        case Qt::Key_PageUp: i_vk = VK_PAGEUP; break;
        case Qt::Key_PageDown: i_vk = VK_PAGEDOWN; break;
        case Qt::Key_Home: i_vk = VK_HOME; break;
        case Qt::Key_End: i_vk = VK_END; break;
        case Qt::Key_Insert: i_vk = VK_INSERT; break;
        case Qt::Key_Delete: i_vk = VK_DELETE; break;
        case Qt::Key_VolumeDown: i_vk = VK_VOLUME_DOWN; break;
        case Qt::Key_VolumeUp: i_vk = VK_VOLUME_UP; break;
        case Qt::Key_MediaTogglePlayPause: i_vk = VK_MEDIA_PLAY_PAUSE; break;
        case Qt::Key_MediaStop: i_vk = VK_MEDIA_STOP; break;
        case Qt::Key_MediaPrevious: i_vk = VK_MEDIA_PREV_TRACK; break;
        case Qt::Key_MediaNext: i_vk = VK_MEDIA_NEXT_TRACK; break;
        default:
            i_vk = toupper( key );
            break;
    }

    if(!RegisterHotKey(NULL, 10, i_mod, i_vk)) {
        hotKey = QKeySequence();
        nativeFilter.setHotKey(hotKey);
        settings.setValue("hotkey", hotKey);
        QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
    }
#elif LINUX
    auto platform = qApp->platformNativeInterface();

    xcb_connection_t *c = static_cast<xcb_connection_t*>(platform->nativeResourceForWindow("connection", 0));
    xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(c);

    if(!hotKey.isEmpty()) {
        // remove previous bindings from all screens
        xcb_screen_iterator_t iter;
        iter = xcb_setup_roots_iterator (xcb_get_setup (c));
        for (; iter.rem; xcb_screen_next (&iter)) {
            xcb_ungrab_key(c, XCB_GRAB_ANY, iter.data->root, XCB_MOD_MASK_ANY);
        }
    }
    hotKey = hotKey_;
    nativeFilter.setHotKey(hotKey);
    settings.setValue("hotkey", hotKey);

    if(hotKey.isEmpty()) return;

    xcb_keysym_t keysym = GetX11Key(hotKey[0]);
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, keysym), keycode;

    if(!keycodes) {
        hotKey = QKeySequence();
        nativeFilter.setHotKey(hotKey);
        settings.setValue("hotkey", hotKey);
        QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
        free(keysyms);
        return;
    }

    // add bindings for all screens
    xcb_screen_iterator_t iter;
    iter = xcb_setup_roots_iterator (xcb_get_setup (c));
    bool any_failed = false;
    for (; iter.rem; xcb_screen_next (&iter)) {
        int i = 0;
        while(keycodes[i] != XCB_NO_SYMBOL) {
            keycode = keycodes[i];
            for(auto modifier : GetX11Modifier(c, keysyms, hotKey[0])) {
                auto cookie = xcb_grab_key_checked(c, true, iter.data->root,
                    modifier, keycode, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_SYNC);
                if(xcb_request_check(c, cookie)) {
                    any_failed = true;
                }
            }
            i += 1;
        }
    }
    if(any_failed) {
        QMessageBox::warning(this, "Key binding warning",
            "Warning: Global hotkey binding problem detected. Some other program might have a conflicting "
            "key binding. If the hotkey doesn't work, try closing some programs or using a different hotkey.");
    }
    free(keysyms);
    free(keycodes);
#endif // WIN32 or LINUX
}

void MainWindow::refreshRequest(){
    ui->treeView->reset();
}

void MainWindow::changeMinFontSize(int minFont){
    ui->webView->settings()->setFontSize(QWebSettings::MinimumFontSize, minFont);
}
