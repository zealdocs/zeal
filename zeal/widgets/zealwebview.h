#ifndef ZEALWEBVIEW_H
#define ZEALWEBVIEW_H

#include <QWebView>

// Subclass QWebView so we can override createWindow
class ZealWebView : public QWebView
{
    Q_OBJECT
public:
    explicit ZealWebView(QWidget *parent = 0);

protected:
    virtual QWebView *createWindow(QWebPage::WebWindowType type);

signals:

public slots:

};

#endif // ZEALWEBVIEW_H
