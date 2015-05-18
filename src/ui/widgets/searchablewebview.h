#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include <QWidget>

#ifdef USE_WEBENGINE
    #define QWebPage QWebEnginePage
#endif

class QLineEdit;
class QWebPage;
class QWebSettings;

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

    int zealZoomFactor() const;
    void setZealZoomFactor(int zf);

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
    void moveLineEdit();

    QLineEdit *m_lineEdit = nullptr;
    WebView *m_webView = nullptr;
    QString m_searchText;
};

#endif // SEARCHABLEWEBVIEW_H
