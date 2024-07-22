/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_BROWSER_WEBVIEW_H
#define ZEAL_BROWSER_WEBVIEW_H

#include <QWebEngineView>

namespace Zeal {
namespace Browser {

class WebView final : public QWebEngineView
{
    Q_OBJECT
    Q_DISABLE_COPY(WebView)
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
