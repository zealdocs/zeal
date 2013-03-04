#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include <QWebView>
#include <QWebSettings>
#include "lineedit.h"

class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = 0);
    void load(const QUrl& url);
    QSize sizeHint() const;
    QWebSettings * settings() const;

protected:
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *);
    
signals:
    
public slots:
    
private:
    LineEdit lineEdit;
    QWebView webView;
    QString searchText;
    void moveLineEdit();
};

#endif // SEARCHABLEWEBVIEW_H
