#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include <QWebView>
#include <QWebSettings>
#include "../lineedit.h"

class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = 0);
    void load(const QUrl& url);
    QSize sizeHint() const;
    QWebSettings * settings() const;
    QWebPage * page() const;
    bool canGoBack();
    bool canGoForward();

protected:
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *);
    
signals:
    void urlChanged(const QUrl &url);
    
public slots:
    void back();
    void forward();
    
private:
    LineEdit lineEdit;
    QWebView webView;
    QString searchText;
    void moveLineEdit();
};

#endif // SEARCHABLEWEBVIEW_H
