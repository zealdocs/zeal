/****************************************************************************
**
** Copyright (C) 2018 Oleg Shparber
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

#ifndef ZEAL_WIDGETUI_SEARCHTOOLBAR_H
#define ZEAL_WIDGETUI_SEARCHTOOLBAR_H

#include <QWidget>

class QLineEdit;
class QToolButton;
class QWebView;

namespace Zeal {
namespace WidgetUi {

class SearchToolBar : public QWidget
{
    Q_OBJECT
public:
    explicit SearchToolBar(QWebView *webView, QWidget *parent = nullptr);

    void setText(const QString &text);
    void activate();

    bool eventFilter(QObject *object, QEvent *event) override;

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    void findNext();
    void findPrevious();

    void hideHighlight();
    void updateHighlight();

    QLineEdit *m_lineEdit = nullptr;
    QToolButton *m_findNextButton = nullptr;
    QToolButton *m_findPreviousButton = nullptr;
    QToolButton *m_highlightAllButton = nullptr;
    QToolButton *m_matchCaseButton = nullptr;

    QWebView *m_webView = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SEARCHTOOLBAR_H
