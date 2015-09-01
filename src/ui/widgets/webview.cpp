/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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

void WebView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::BackButton:
        back();
        event->accept();
        break;
    case Qt::ForwardButton:
        forward();
        event->accept();
        break;
    default:
        break;
    }

    QWebView::mousePressEvent(event);
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
