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

#include "searchablewebview.h"

#include "webview.h"

#include <QLineEdit>
#include <QStyle>
#include <QResizeEvent>

#ifdef USE_WEBENGINE
#include <QWebEngineHistory>
#include <QWebEnginePage>
#else
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#endif

SearchableWebView::SearchableWebView(QWidget *parent) :
    QWidget(parent),
    m_searchLineEdit(new QLineEdit(this)),
    m_webView(new WebView(this))
{
    m_webView->setAttribute(Qt::WA_AcceptTouchEvents, false);

    m_searchLineEdit->hide();
    m_searchLineEdit->installEventFilter(this);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &SearchableWebView::find);

    connect(m_webView, &QWebView::loadFinished, [&](bool ok) {
        Q_UNUSED(ok)
        moveLineEdit();
    });

    connect(m_webView, &QWebView::urlChanged, this, &SearchableWebView::urlChanged);
    connect(m_webView, &QWebView::titleChanged, this, &SearchableWebView::titleChanged);
#ifdef USE_WEBENGINE
    // not implemented?
    // connect(m_webView->page(), &QWebPage::linkClicked, this, &SearchableWebView::linkClicked);
#else
    connect(m_webView, &QWebView::linkClicked, this, &SearchableWebView::linkClicked);
#endif
}

void SearchableWebView::setPage(QWebPage *page)
{
    m_webView->setPage(page);

    connect(page, &QWebPage::linkHovered, [&](const QString &link) {
        if (!link.startsWith(QLatin1String("file:")))
            setToolTip(link);
    });
}

int SearchableWebView::zoomFactor() const
{
    return m_webView->zealZoomFactor();
}

void SearchableWebView::setZoomFactor(int value)
{
    m_webView->setZealZoomFactor(value);
}

bool SearchableWebView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_searchLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            hideSearchBar();
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            findNext(m_searchLineEdit->text(), keyEvent->modifiers() & Qt::ShiftModifier);
            return true;
        default:
            break;
        }
    }

    return QWidget::eventFilter(object, event);
}

void SearchableWebView::load(const QUrl &url)
{
    m_webView->load(url);
}

void SearchableWebView::focus()
{
    m_webView->setFocus();
}

QWebPage *SearchableWebView::page() const
{
    return m_webView->page();
}

QSize SearchableWebView::sizeHint() const
{
    return m_webView->sizeHint();
}

void SearchableWebView::back()
{
    m_webView->back();
}

void SearchableWebView::forward()
{
    m_webView->forward();
}

void SearchableWebView::showSearchBar()
{
    m_searchLineEdit->show();
    m_searchLineEdit->setFocus();
    if (!m_searchLineEdit->text().isEmpty()) {
        m_searchLineEdit->selectAll();
        find(m_searchLineEdit->text());
    }
}

void SearchableWebView::hideSearchBar()
{
    m_searchLineEdit->hide();
#ifdef USE_WEBENGINE
    m_webView->findText(QString());
#else
    m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
#endif
}

bool SearchableWebView::canGoBack() const
{
    return m_webView->history()->canGoBack();
}

bool SearchableWebView::canGoForward() const
{
    return m_webView->history()->canGoForward();
}

void SearchableWebView::keyPressEvent(QKeyEvent *event)
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

void SearchableWebView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_webView->resize(event->size().width(), event->size().height());
    moveLineEdit();
}

void SearchableWebView::find(const QString &text)
{
#ifdef USE_WEBENGINE
    // FIXME: There's no way to just show highlight when search term is already selected.
    // So we need a workaround before switching to Qt WebEngine.
    m_webView->findText(text);
#else
    if (m_webView->selectedText() != text) {
        m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
        m_webView->findText(QString());
        if (text.isEmpty())
            return;

        m_webView->findText(text, QWebPage::FindWrapsAroundDocument);
    }

    m_webView->findText(text, QWebPage::HighlightAllOccurrences);
#endif
}

void SearchableWebView::findNext(const QString &text, bool backward)
{
#ifdef USE_WEBENGINE
    QWebPage::FindFlags flags = 0;
#else
    QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
#endif
    if (backward)
        flags |= QWebPage::FindBackward;
    m_webView->findText(text, flags);
}

void SearchableWebView::moveLineEdit()
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#ifdef USE_WEBENGINE
    // FIXME: scrollbar width
#else
    frameWidth += m_webView->page()->currentFrame()->scrollBarGeometry(Qt::Vertical).width();
#endif
    m_searchLineEdit->move(rect().right() - frameWidth - m_searchLineEdit->sizeHint().width(), rect().top());
    m_searchLineEdit->raise();
}
