#include "zealwebview.h"

#include "../mainwindow.h"

#include <QApplication>
#include <QWheelEvent>

ZealWebView::ZealWebView(QWidget *parent) :
    QWebView(parent)
{
}

int ZealWebView::zealZoomFactor() const
{
    return m_zoomFactor;
}

void ZealWebView::setZealZoomFactor(int zf)
{
    m_zoomFactor = zf;
    updateZoomFactor();
}

QWebView *ZealWebView::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type)

    MainWindow *mw = qobject_cast<MainWindow *>(qApp->activeWindow());
    mw->createTab();
    return this;
}

void ZealWebView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        m_zoomFactor += event->delta() / 120;
        updateZoomFactor();
    } else {
        QWebView::wheelEvent(event);
    }
}

void ZealWebView::updateZoomFactor()
{
    setZoomFactor(1 + m_zoomFactor / 10.);
}
