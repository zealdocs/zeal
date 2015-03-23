#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include "webview.h"

#include <QLineEdit>

#ifdef USE_WEBENGINE
    #include <QWebEngineView>
    #include <QWebEngineSettings>
    #define QWebSettings QWebEngineSettings
#else
    #include <QWebView>
    #include <QWebSettings>
#endif

class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = nullptr);

    void load(const QUrl &url);
    void focus();
    QSize sizeHint() const override;
    QWebSettings *settings() const;
    QWebPage *page() const;
    bool canGoBack();
    bool canGoForward();
    void setPage(QWebPage *page);
    int zealZoomFactor()
    {
        return webView.zealZoomFactor();
    }

    void setZealZoomFactor(int zf_)
    {
        webView.setZealZoomFactor(zf_);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *) override;

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void linkClicked(const QUrl &url);

public slots:
    void back();
    void forward();

private:
    QLineEdit lineEdit;
    WebView webView;
    QString searchText;
    void moveLineEdit();
};

#endif // SEARCHABLEWEBVIEW_H
