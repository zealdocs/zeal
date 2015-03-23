#include "webview.h"

#include "../mainwindow.h"

#include <QApplication>
#include <QWheelEvent>

WebView::WebView(QWidget *parent) :
    QWebView(parent)
{
}

int WebView::zealZoomFactor() const
{
    return m_zoomFactor;
}

void WebView::setZealZoomFactor(int zf)
{
    m_zoomFactor = zf;
    updateZoomFactor();
}

QWebView *WebView::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type)

    MainWindow *mw = qobject_cast<MainWindow *>(qApp->activeWindow());
    mw->createTab();
    return this;
}

void WebView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        m_zoomFactor += event->delta() / 120;
        updateZoomFactor();
    } else {
        QWebView::wheelEvent(event);
    }
}

void WebView::updateZoomFactor()
{
    setZoomFactor(1 + m_zoomFactor / 10.);
}
