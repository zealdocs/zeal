#include "zealwebview.h"
#include "../mainwindow.h"

int zf = 0;

ZealWebView::ZealWebView(QWidget *parent) : QWebView(parent) {};

QWebView *ZealWebView::createWindow(QWebPage::WebWindowType type) {
    Q_UNUSED(type)

    MainWindow *mw = qobject_cast<MainWindow *>(qApp->activeWindow());
    mw->createTab();
    return this;
}

void ZealWebView::wheelEvent(QWheelEvent *event){
    if (event->modifiers() & Qt::ControlModifier){
        zf += event->delta()/120;
        setZoomFactor(1+(qreal(zf)/10));
    }
    else{
        QWebView::wheelEvent(event);
    }
}
