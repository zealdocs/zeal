#include <QShortcut>
#include <QStyle>
#include <QResizeEvent>
#include <QWebFrame>
#include <QWebHistory>
#include "searchablewebview.h"


SearchableWebView::SearchableWebView(QWidget *parent) :
    QWidget(parent), lineEdit(this), webView(this)
{
    lineEdit.hideOnClear = true;
    lineEdit.hide();
    connect(&lineEdit, &LineEdit::textChanged, [&](const QString& text) {
        // clear selection:
        webView.findText("");
        webView.findText("", QWebPage::HighlightAllOccurrences);
        if(!text.isEmpty()) {
            // select&scroll to one occurence:
            webView.findText(text, QWebPage::FindWrapsAroundDocument);
            // highlight other occurences:
            webView.findText(text, QWebPage::HighlightAllOccurrences);
        }
        // store text for later searches
        searchText = text;
    });

    auto shortcut = new QShortcut(QKeySequence::Find, this);
    connect(shortcut, &QShortcut::activated, [&] {
        lineEdit.show();
        lineEdit.setFocus();
    });

    connect(&webView, &QWebView::loadFinished, [&](bool ok) {
        moveLineEdit();
    });

    connect(&webView, &QWebView::urlChanged, this, &SearchableWebView::urlChanged);
    connect(&webView, &QWebView::titleChanged, this, &SearchableWebView::titleChanged);

    connect(&webView, &QWebView::loadStarted, [&]() {
        lineEdit.clear();
    });

    // Display tooltip showing link location when hovered over.
    connect(webView.page(), &QWebPage::linkHovered, [&](const QString &link, const QString &title, const QString &textContent){
        if( !link.startsWith("file:///") ){
            setToolTip( link );
        }
    });
}

void SearchableWebView::moveLineEdit() {
    QSize sz = lineEdit.sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    frameWidth += webView.page()->currentFrame()->scrollBarGeometry(Qt::Vertical).width();
    lineEdit.move(rect().right() - frameWidth - sz.width(), rect().top());
    lineEdit.raise();
}

void SearchableWebView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    webView.resize(event->size().width(), event->size().height());
    moveLineEdit();
}

void SearchableWebView::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
        if( event->modifiers() & Qt::ShiftModifier )
            flags |= QWebPage::FindBackward;
        webView.findText(searchText, flags);
    }
    if(event->key() == Qt::Key_Escape) {
        lineEdit.clear();
    }
}

void SearchableWebView::load(const QUrl &url) {
    webView.load(url);
}

void SearchableWebView::focus()
{
    webView.setFocus();
}

QWebPage * SearchableWebView::page() const {
    return webView.page();
}

QSize SearchableWebView::sizeHint() const {
    return webView.sizeHint();
}

QWebSettings * SearchableWebView::settings() const {
    return webView.settings();
}

void SearchableWebView::back() {
    webView.back();
}

void SearchableWebView::forward() {
    webView.forward();
}

bool SearchableWebView::canGoBack() {
    return webView.history()->canGoBack();
}

bool SearchableWebView::canGoForward() {
    return webView.history()->canGoForward();
}
