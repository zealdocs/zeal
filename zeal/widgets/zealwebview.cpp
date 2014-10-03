#include "zealwebview.h"
#include "../mainwindow.h"

ZealWebView::ZealWebView(QWidget *parent) : QWebView(parent) {};

QWebView *ZealWebView::createWindow(QWebPage::WebWindowType type) {
    Q_UNUSED(type)

    MainWindow *mw = qobject_cast<MainWindow *>(qApp->activeWindow());
    mw->createTab();
    return this;
}
