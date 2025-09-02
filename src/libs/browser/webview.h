// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_WEBVIEW_H
#define ZEAL_BROWSER_WEBVIEW_H

#include <QWebEngineView>

namespace Zeal {
namespace Browser {

class WebView final : public QWebEngineView
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebView)
public:
    explicit WebView(QWidget *parent = nullptr);

    int zoomLevel() const;
    void setZoomLevel(int level);

    bool eventFilter(QObject *watched, QEvent *event) override;

    static const QVector<int> &availableZoomLevels();
    static int defaultZoomLevel();

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void zoomLevelChanged();

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    bool handleMouseReleaseEvent(QMouseEvent *event);
    bool handleWheelEvent(QWheelEvent *event);

    QMenu *m_contextMenu = nullptr;
    QUrl m_clickedLink;
    int m_zoomLevel = 0;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_WEBVIEW_H
