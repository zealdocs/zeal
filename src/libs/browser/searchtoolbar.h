/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_BROWSER_SEARCHTOOLBAR_H
#define ZEAL_BROWSER_SEARCHTOOLBAR_H

#include <QWidget>

class QLineEdit;
class QToolButton;
class QWebEngineView;

namespace Zeal {
namespace Browser {

class SearchToolBar final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SearchToolBar)
public:
    explicit SearchToolBar(QWebEngineView *webView, QWidget *parent = nullptr);

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

    QWebEngineView *m_webView = nullptr;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_SEARCHTOOLBAR_H
