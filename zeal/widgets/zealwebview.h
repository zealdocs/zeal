#ifndef ZEALWEBVIEW_H
#define ZEALWEBVIEW_H

#ifdef USE_WEBENGINE
    #include <QWebEngineView>
    #include <QWebEnginePage>
    #define QWebView QWebEngineView
    #define QWebPage QWebEnginePage
#else
    #include <QWebView>
#endif

// Subclass QWebView so we can override createWindow
class ZealWebView : public QWebView
{
    Q_OBJECT
public:
    explicit ZealWebView(QWidget *parent = 0);
    void wheelEvent(QWheelEvent * event);
    int zealZoomFactor() {
        return zf;
    }
    void setZealZoomFactor(int zf_) {
        zf = zf_;
        updateZoomFactor();
    }
protected:
    virtual QWebView *createWindow(QWebPage::WebWindowType type);
private:
    int zf = 0;
    void updateZoomFactor() {
        setZoomFactor(1+(qreal(zf)/10));
    }

signals:

public slots:

};

#endif // ZEALWEBVIEW_H
