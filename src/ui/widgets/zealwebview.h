#ifndef WEBVIEW_H
#define WEBVIEW_H

#ifdef USE_WEBENGINE
    #include <QWebEngineView>
    #include <QWebEnginePage>
    #define QWebView QWebEngineView
    #define QWebPage QWebEnginePage
#else
    #include <QWebView>
#endif

class ZealWebView : public QWebView
{
    Q_OBJECT
public:
    explicit ZealWebView(QWidget *parent = nullptr);

    int zealZoomFactor() const;
    void setZealZoomFactor(int zf);

protected:
    QWebView *createWindow(QWebPage::WebWindowType type) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updateZoomFactor();

    int m_zoomFactor = 0;
};

#endif // WEBVIEW_H
