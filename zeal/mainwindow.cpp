#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zeallistmodel.h"
#include "zealsearchmodel.h"
#include "zealdocsetsregistry.h"
#include "zealsearchitemdelegate.h"
#include "zealsettingsdialog.h"

#include <QDebug>
#include <QAbstractEventDispatcher>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QLocalSocket>
#include <QDir>
#include <QSettings>
#include <QTimer>

#ifdef WIN32
#include <windows.h>
#else
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
#include <QtGui/5.1.0/QtGui/qpa/qplatformnativeinterface.h>
#else
#include <QtGui/5.0.0/QtGui/qpa/qplatformnativeinterface.h>
#endif
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include "xcb_keysym.h"
#endif

const QString serverName = "zeal_process_running";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings("Zeal", "Zeal")
{
    // server for detecting already running instances
    localServer = new QLocalServer(this);
    connect(localServer, &QLocalServer::newConnection, [&]() {
        bringToFront(false);
    });
    QLocalServer::removeServer(serverName);  // remove in case previous instance crashed
    localServer->listen(serverName);

#ifndef WIN32
    // Default style sometimes (when =windows) doesn't work well with Linux
    qApp->setStyle(QStyleFactory::create("fusion"));
#endif

    // initialise icons
#ifdef WIN32
    icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation);
#else
    icon = QIcon::fromTheme("edit-find");
#endif
    setWindowIcon(icon);
    createTrayIcon();

    QKeySequence keySequence;
    if(settings.value("hotkey").isNull()) {
        keySequence = QKeySequence("Alt+Space");
    } else {
        keySequence = settings.value("hotkey").value<QKeySequence>();
    }

    // initialise key grabber
    connect(&nativeFilter, &ZealNativeEventFilter::gotHotKey, [&]() {
        if(isVisible()) hide();
        else {
            bringToFront(true);
        }
    });
    qApp->eventDispatcher()->installNativeEventFilter(&nativeFilter);
    setHotKey(keySequence);

    // initialise docsets
    auto dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    auto dataDir = QDir(dataLocation);
    if(!dataDir.cd("docsets")) {
        QMessageBox::critical(this, "No docsets directory found",
                              QString("'docsets' directory not found in '%1'").arg(dataLocation));
    } else {
        for(auto subdir : dataDir.entryInfoList()) {
            if(subdir.isDir() && subdir.fileName() != "." && subdir.fileName() != "..") {
                QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                          Q_ARG(QString, subdir.absoluteFilePath()));
            }
        }
    }

    // initialise ui
    ui->setupUi(this);
    restoreGeometry(settings.value("geometry").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        settings.setValue("splitter", ui->splitter->saveState());
    });

    // menu
    auto quitAction = ui->menuBar->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, [=]() { settings.setValue("geometry", saveGeometry()); });
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    auto settingsAction = ui->menuBar->addAction("&Options");
    connect(settingsAction, &QAction::triggered, [=]() {
        ZealSettingsDialog settingsDialog;
        settingsDialog.setHotKey(hotKey);
        nativeFilter.setEnabled(false);
        if(settingsDialog.exec()) {
            setHotKey(settingsDialog.hotKey());
        }
        nativeFilter.setEnabled(true);
    });
    auto helpMenu = new QMenu("&Help");
    auto aboutAction = helpMenu->addAction("&About");
    auto aboutQtAction = helpMenu->addAction("About &Qt");
    connect(aboutAction, &QAction::triggered,
            [&]() { QMessageBox::about(this, "About Zeal",
                "This is Zeal - a documentation browser.\n\n"
                "For details see https://github.com/jkozera/zeal/"); });
    connect(aboutQtAction, &QAction::triggered,
            [&]() { QMessageBox::aboutQt(this); });
    ui->menuBar->addMenu(helpMenu);

    // treeView and lineEdit
    ui->lineEdit->setTreeView(ui->treeView);
    ui->lineEdit->setFocus();
    ui->treeView->setModel(&zealList);
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setItemDelegate(new ZealSearchItemDelegate(ui->treeView, ui->lineEdit, ui->treeView));
    connect(ui->treeView, &QTreeView::activated, [&](const QModelIndex& index) {
        QStringList url_l = index.sibling(index.row(), 1).data().toString().split('#');
        QUrl url = QUrl::fromLocalFile(url_l[0]);
        if(url_l.count() > 1) {
            url.setFragment(url_l[1]);
        }
        ui->webView->load(url);
    });
    connect(&zealSearch, &ZealSearchModel::queryCompleted, [&]() {
        ui->treeView->setModel(&zealSearch);
        ui->treeView->reset();
        ui->treeView->setColumnHidden(1, true);
        ui->treeView->setCurrentIndex(zealSearch.index(0, 0, QModelIndex()));
    });
    connect(ui->lineEdit, &QLineEdit::textChanged, [&](const QString& text) {
        if(!text.isEmpty()) {
            zealSearch.setQuery(text);
        } else {
            ui->treeView->setModel(&zealList);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete localServer;
}

void MainWindow::createTrayIcon()
{
    auto trayIconMenu = new QMenu(this);
    auto quitAction = trayIconMenu->addAction("&Quit");
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    auto trayIcon = new QSystemTrayIcon(this);
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
    int key = hotKey[hotKey.count()-1];
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
#else
    auto platform = qApp->platformNativeInterface();

    xcb_connection_t *c = static_cast<xcb_connection_t*>(platform->nativeResourceForWindow("connection", 0));
    xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(c);

    if(!hotKey.isEmpty()) {
        // disable previous hotkey
        xcb_keysym_t keysym = GetX11Key(hotKey[hotKey.count()-1]);
        xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, keysym), keycode;

        // remove bindings from all screens
        xcb_screen_iterator_t iter;
        iter = xcb_setup_roots_iterator (xcb_get_setup (c));
        for (; iter.rem; xcb_screen_next (&iter)) {
            int i = 0;
            while(keycodes[i] != XCB_NO_SYMBOL) {
                keycode = keycodes[i];
                xcb_ungrab_key(c, keycode, iter.data->root, XCB_MOD_MASK_ANY);
                i += 1;
            }
        }

        free(keycodes);
    }
    hotKey = hotKey_;
    nativeFilter.setHotKey(hotKey);
    settings.setValue("hotkey", hotKey);

    if(hotKey.isEmpty()) return;

    xcb_keysym_t keysym = GetX11Key(hotKey[hotKey.count()-1]);
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, keysym), keycode;

    if(!keycodes) {
        hotKey = QKeySequence();
        nativeFilter.setHotKey(hotKey);
        settings.setValue("hotkey", hotKey);
        QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
        return;
    }

    // add bindings for all screens
    xcb_screen_iterator_t iter;
    iter = xcb_setup_roots_iterator (xcb_get_setup (c));
    for (; iter.rem; xcb_screen_next (&iter)) {
        int i = 0;
        while(keycodes[i] != XCB_NO_SYMBOL) {
            keycode = keycodes[i];
            xcb_void_cookie_t cookie = xcb_grab_key_checked(c, true, iter.data->root, XCB_MOD_MASK_ANY, keycode, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_SYNC);
            if(xcb_request_check(c, cookie)) {
                hotKey = QKeySequence();
                nativeFilter.setHotKey(hotKey);
                settings.setValue("hotkey", hotKey);
                QMessageBox::warning(this, "Key binding failed", "Binding global hotkey failed.");
                return;
            }
            i += 1;
        }
    }
    free(keysyms);
    free(keycodes);
#endif // WIN32
}
