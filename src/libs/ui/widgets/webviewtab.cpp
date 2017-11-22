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

#include "webview.h"

#include <QCoreApplication>
#include <QLineEdit>
#include <QStyle>
#include <QResizeEvent>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#include <QVBoxLayout>

using namespace Zeal::WidgetUi;

WebViewTab::WebViewTab(QWidget *parent) :
    QWidget(parent),
    m_searchLineEdit(new QLineEdit(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_searchLineEdit->hide();
    m_searchLineEdit->installEventFilter(this);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &WebViewTab::find);

    m_webView = new WebView();
    connect(m_webView, &QWebView::loadFinished, this, [this](bool ok) {
        Q_UNUSED(ok)
        moveLineEdit();
    });

    connect(m_webView->page(), &QWebPage::linkHovered, [this](const QString &link) {
        if (link.startsWith(QLatin1String("file:")) || link.startsWith(QLatin1String("qrc:")))
            return;

        setToolTip(link);
    });

    connect(m_webView, &QWebView::linkClicked, this, &WebViewTab::linkClicked);
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

bool WebViewTab::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_searchLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            hideSearchBar();
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            findNext(m_searchLineEdit->text(), keyEvent->modifiers() & Qt::ShiftModifier);
            return true;
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            QCoreApplication::sendEvent(m_webView, event);
            return true;
        default:
            break;
        }
    }

    return QWidget::eventFilter(object, event);
}

void WebViewTab::load(const QUrl &url)
{
    m_webView->load(url);
}

void WebViewTab::focus()
{
    m_webView->setFocus();
}

QSize WebViewTab::sizeHint() const
{
    return m_webView->sizeHint();
}

void WebViewTab::back()
{
    m_webView->back();
}

void WebViewTab::forward()
{
    m_webView->forward();
}

void WebViewTab::showSearchBar()
{
    m_searchLineEdit->show();
    m_searchLineEdit->setFocus();
    if (!m_searchLineEdit->text().isEmpty()) {
        m_searchLineEdit->selectAll();
        find(m_searchLineEdit->text());
    }
}

void WebViewTab::hideSearchBar()
{
    m_searchLineEdit->hide();
    m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
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
        showSearchBar();
        event->accept();
        break;
    default:
        event->ignore();
        break;
    }
}

void WebViewTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_webView->resize(event->size().width(), event->size().height());
    moveLineEdit();
}

void WebViewTab::find(const QString &text)
{
    if (m_webView->selectedText() != text) {
        m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
        m_webView->findText(QString());
        if (text.isEmpty())
            return;

        m_webView->findText(text, QWebPage::FindWrapsAroundDocument);
    }

    m_webView->findText(text, QWebPage::HighlightAllOccurrences);
}

void WebViewTab::findNext(const QString &text, bool backward)
{
    QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
    if (backward)
        flags |= QWebPage::FindBackward;

    m_webView->findText(text, flags);
}

void WebViewTab::moveLineEdit()
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    frameWidth += m_webView->page()->currentFrame()->scrollBarGeometry(Qt::Vertical).width();

    m_searchLineEdit->move(rect().right() - frameWidth - m_searchLineEdit->sizeHint().width(), rect().top());
    m_searchLineEdit->raise();
}
