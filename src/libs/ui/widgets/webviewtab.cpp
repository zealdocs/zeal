/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "webviewtab.h"

#include "searchtoolbar.h"
#include "webview.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QStyle>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#include <QVBoxLayout>

using namespace Zeal::WidgetUi;

WebViewTab::WebViewTab(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_webView = new WebView();
    connect(m_webView->page(), &QWebPage::linkHovered, [this](const QString &link) {
        if (link.startsWith(QLatin1String("file:")) || link.startsWith(QLatin1String("qrc:")))
            return;

        setToolTip(link);
    });

    connect(m_webView, &QWebView::titleChanged, this, &WebViewTab::titleChanged);
    connect(m_webView, &QWebView::urlChanged, this, &WebViewTab::urlChanged);

    layout->addWidget(m_webView);

    setLayout(layout);

}

int WebViewTab::zoomLevel() const
{
    return m_webView->zoomLevel();
}

void WebViewTab::setZoomLevel(int level)
{
    m_webView->setZoomLevel(level);
}

void WebViewTab::setWebBridgeObject(const QString &name, QObject *object)
{
    connect(m_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
            this, [=]() {
        m_webView->page()->mainFrame()->addToJavaScriptWindowObject(name, object);
    });
}

void WebViewTab::load(const QUrl &url)
{
    m_webView->load(url);
}

void WebViewTab::focus()
{
    m_webView->setFocus();
}

void WebViewTab::activateSearchBar()
{
    if (m_searchToolBar == nullptr) {
        m_searchToolBar = new SearchToolBar(m_webView);
        layout()->addWidget(m_searchToolBar);
    }

    if (m_webView->hasSelection()) {
        const QString selectedText = m_webView->selectedText().simplified();
        if (!selectedText.isEmpty()) {
            m_searchToolBar->setText(selectedText);
        }
    }

    m_searchToolBar->activate();
}

void WebViewTab::back()
{
    m_webView->back();
}

void WebViewTab::forward()
{
    m_webView->forward();
}

bool WebViewTab::canGoBack() const
{
    return m_webView->history()->canGoBack();
}

bool WebViewTab::canGoForward() const
{
    return m_webView->history()->canGoForward();
}

QString WebViewTab::title() const
{
    return m_webView->title();
}

QUrl WebViewTab::url() const
{
    return m_webView->url();
}

QWebHistory *WebViewTab::history() const
{
    return m_webView->history();
}

void WebViewTab::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Slash:
        activateSearchBar();
        break;
    default:
        event->ignore();
        break;
    }
}
