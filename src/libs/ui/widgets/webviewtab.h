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

#ifndef ZEAL_WIDGETUI_WEBVIEWTAB_H
#define ZEAL_WIDGETUI_WEBVIEWTAB_H

#include <QWidget>

class QLineEdit;
class QWebHistory;

namespace Zeal {
namespace WidgetUi {

class WebView;

class WebViewTab : public QWidget
{
    Q_OBJECT
public:
    explicit WebViewTab(QWidget *parent = nullptr);

    void load(const QUrl &url);
    void focus();
    QSize sizeHint() const override;
    bool canGoBack() const;
    bool canGoForward() const;

    QString title() const;
    QUrl url() const;

    QWebHistory *history() const;

    int zoomLevel() const;
    void setZoomLevel(int level);

    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void linkClicked(const QUrl &url);
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);

public slots:
    void back();
    void forward();
    void showSearchBar();
    void hideSearchBar();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void find(const QString &text);
    void findNext(const QString &text, bool backward = false);
    void moveLineEdit();

    QLineEdit *m_searchLineEdit = nullptr;
    WebView *m_webView = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_WEBVIEWTAB_H
