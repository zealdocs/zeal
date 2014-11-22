#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#ifdef USE_WEBENGINE
    #include <QWebEngineView>
    #include <QWebEngineSettings>
    #define QWebSettings QWebEngineSettings
#else
    #include <QWebView>
    #include <QWebSettings>
#endif
#include "zealwebview.h"
#include "../lineedit.h"

class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = 0);
    void load(const QUrl& url);
    void focus();
    QSize sizeHint() const;
    QWebSettings * settings() const;
    QWebPage * page() const;
    bool canGoBack();
    bool canGoForward();
    void setPage(QWebPage *page);
    int zealZoomFactor() { return webView.zealZoomFactor(); }
    void setZealZoomFactor(int zf_) { webView.setZealZoomFactor(zf_); }

protected:
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *);

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void linkClicked(const QUrl &url);

public slots:
    void back();
    void forward();

private:
    LineEdit lineEdit;
    ZealWebView webView;
    QString searchText;
    void moveLineEdit();
};

#endif // SEARCHABLEWEBVIEW_H
