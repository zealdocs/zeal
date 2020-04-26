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

#ifndef ZEAL_BROWSER_WEBCONTROL_H
#define ZEAL_BROWSER_WEBCONTROL_H

#include <QWidget>

class QWebEngineHistory;

namespace Zeal {
namespace Browser {

class SearchToolBar;
class WebView;

class WebControl final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(WebControl)
public:
    explicit WebControl(QWidget *parent = nullptr);

    void focus();
    void load(const QUrl &url);
    bool canGoBack() const;
    bool canGoForward() const;

    QString title() const;
    QUrl url() const;

    QWebEngineHistory *history() const;
    void restoreHistory(const QByteArray &array);
    QByteArray saveHistory() const;

    int zoomLevel() const;
    void setZoomLevel(int level);
    void setJavaScriptEnabled(bool enabled);

    void setWebBridgeObject(const QString &name, QObject *object);

signals:
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);

public slots:
    void activateSearchBar();
    void back();
    void forward();

    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    friend class WebView;

    WebView *m_webView = nullptr;
    SearchToolBar *m_searchToolBar = nullptr;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_WEBCONTROL_H
