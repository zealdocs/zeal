#include "searchablewebview.h"

#include <QShortcut>
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
    lineEdit(this),
    webView(this)
{
    webView.setAttribute(Qt::WA_AcceptTouchEvents, false);
    lineEdit.hideOnClear = true;
    lineEdit.hide();
    connect(&lineEdit, &LineEdit::textChanged, [&](const QString &text) {
        // clear selection:
#ifdef USE_WEBENGINE
        webView.findText(text);
#else
        webView.findText("");
        webView.findText("", QWebPage::HighlightAllOccurrences);
        if (!text.isEmpty()) {
            // select&scroll to one occurence:
            webView.findText(text, QWebPage::FindWrapsAroundDocument);
            // highlight other occurences:
            webView.findText(text, QWebPage::HighlightAllOccurrences);
        }
#endif

        // store text for later searches
        searchText = text;
    });

    auto shortcut = new QShortcut(QKeySequence::Find, this);
    connect(shortcut, &QShortcut::activated, [&] {
        lineEdit.show();
        lineEdit.setFocus();
    });

    connect(&webView, &QWebView::loadFinished, [&](bool ok) {
        Q_UNUSED(ok)
        moveLineEdit();
    });

    connect(&webView, &QWebView::urlChanged, this, &SearchableWebView::urlChanged);
    connect(&webView, &QWebView::titleChanged, this, &SearchableWebView::titleChanged);
#ifdef USE_WEBENGINE
    // not implemented?
    // connect(webView.page(), &QWebPage::linkClicked, this, &SearchableWebView::linkClicked);
#else
    connect(&webView, &QWebView::linkClicked, this, &SearchableWebView::linkClicked);
#endif

    connect(&webView, &QWebView::loadStarted, [&]() {
        lineEdit.clear();
    });

    // Display tooltip showing link location when hovered over.
#ifdef USE_WEBENGINE
    connect(webView.page(), &QWebPage::linkHovered, [&](const QString &link) {
#else
    connect(webView.page(), &QWebPage::linkHovered,
            [&](const QString &link, const QString &title, const QString &textContent) {
        Q_UNUSED(title)
        Q_UNUSED(textContent)
#endif
        if (!link.startsWith("file:///"))
            setToolTip(link);
    });
}

void SearchableWebView::setPage(QWebPage *page)
{
    webView.setPage(page);
}

void SearchableWebView::moveLineEdit()
{
    QSize sz = lineEdit.sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#ifdef USE_WEBENGINE
    // FIXME scrollbar width
#else
    frameWidth += webView.page()->currentFrame()->scrollBarGeometry(Qt::Vertical).width();
#endif
    lineEdit.move(rect().right() - frameWidth - sz.width(), rect().top());
    lineEdit.raise();
}

void SearchableWebView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    webView.resize(event->size().width(), event->size().height());
    moveLineEdit();
}

void SearchableWebView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
#ifdef USE_WEBENGINE
        QWebPage::FindFlags flags = 0;
#else
        QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
#endif
        if (event->modifiers() & Qt::ShiftModifier)
            flags |= QWebPage::FindBackward;
        webView.findText(searchText, flags);
    }

    if (event->key() == Qt::Key_Slash) {
        lineEdit.show();
        lineEdit.setFocus();
    }

    // Ignore all other events and pass them to the parent widget.
    event->ignore();
}

void SearchableWebView::load(const QUrl &url)
{
    webView.load(url);
}

void SearchableWebView::focus()
{
    webView.setFocus();
}

QWebPage *SearchableWebView::page() const
{
    return webView.page();
}

QSize SearchableWebView::sizeHint() const
{
    return webView.sizeHint();
}

QWebSettings *SearchableWebView::settings() const
{
    return webView.settings();
}

void SearchableWebView::back()
{
    webView.back();
}

void SearchableWebView::forward()
{
    webView.forward();
}

bool SearchableWebView::canGoBack()
{
    return webView.history()->canGoBack();
}

bool SearchableWebView::canGoForward()
{
    return webView.history()->canGoForward();
}
