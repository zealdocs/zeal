#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include <QWidget>

#ifdef USE_WEBENGINE
    #define QWebPage QWebEnginePage
#endif

class QLineEdit;
class QWebPage;

class WebView;

class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = nullptr);

    void load(const QUrl &url);
    void focus();
    QSize sizeHint() const override;
    QWebPage *page() const;
    bool canGoBack() const;
    bool canGoForward() const;
    void setPage(QWebPage *page);

    int zoomFactor() const;
    void setZoomFactor(int value);

    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void linkClicked(const QUrl &url);

public slots:
    void back();
    void forward();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void showSearch();
    void hideSearch();
    void find(const QString &text);
    void findNext(const QString &text, bool backward = false);
    void moveLineEdit();

    QLineEdit *m_searchLineEdit = nullptr;
    WebView *m_webView = nullptr;
};

#endif // SEARCHABLEWEBVIEW_H
