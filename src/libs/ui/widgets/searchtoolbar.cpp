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

#include "searchtoolbar.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QKeyEvent>
#include <QStyle>
#include <QToolButton>
#include <QWebPage>
#include <QWebView>

using namespace Zeal::WidgetUi;

SearchToolBar::SearchToolBar(QWebView *webView, QWidget *parent)
    : QWidget(parent)
    , m_webView(webView)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_lineEdit = new QLineEdit();
    m_lineEdit->installEventFilter(this);
    m_lineEdit->setPlaceholderText(tr("Find in page"));
    m_lineEdit->setMaximumWidth(200);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchToolBar::findNext);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchToolBar::updateHighlight);
    layout->addWidget(m_lineEdit);

    m_findPreviousButton = new QToolButton();
    m_findPreviousButton->setAutoRaise(true);
    m_findPreviousButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowBack));
    m_findPreviousButton->setToolTip(tr("Previous result"));
    connect(m_findPreviousButton, &QToolButton::clicked, this, &SearchToolBar::findPrevious);
    layout->addWidget(m_findPreviousButton);

    // A workaround for QAbstractButton lacking support for multiple shortcuts.
    QAction *action = new QAction(m_findPreviousButton);
    action->setShortcuts(QKeySequence::FindPrevious);
    // TODO: Investigate why direct connection does not work.
    //connect(action, &QAction::triggered, m_findPreviousButton, &QToolButton::animateClick);
    connect(action, &QAction::triggered, this, [this]() { m_findPreviousButton->animateClick(); });
    addAction(action);

    m_findNextButton = new QToolButton();
    m_findNextButton->setAutoRaise(true);
    m_findNextButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowForward));
    m_findNextButton->setToolTip(tr("Next result"));
    connect(m_findNextButton, &QToolButton::clicked, this, &SearchToolBar::findNext);
    layout->addWidget(m_findNextButton);

    action = new QAction(m_findNextButton);
    action->setShortcuts(QKeySequence::FindNext);
    connect(action, &QAction::triggered, this, [this]() { m_findNextButton->animateClick(); });
    addAction(action);

    m_highlightAllButton = new QToolButton();
    m_highlightAllButton->setAutoRaise(true);
    m_highlightAllButton->setCheckable(true);
    m_highlightAllButton->setText(tr("High&light All"));
    connect(m_highlightAllButton, &QToolButton::toggled, this, &SearchToolBar::updateHighlight);
    layout->addWidget(m_highlightAllButton);

    m_matchCaseButton = new QToolButton();
    m_matchCaseButton->setAutoRaise(true);
    m_matchCaseButton->setCheckable(true);
    m_matchCaseButton->setText(tr("Mat&ch Case"));
    connect(m_matchCaseButton, &QToolButton::toggled, this, &SearchToolBar::updateHighlight);
    layout->addWidget(m_matchCaseButton);

    layout->addStretch();

    QToolButton *closeButton = new QToolButton();
    closeButton->setAutoRaise(true);
    closeButton->setIcon(qApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    closeButton->setToolTip(tr("Close find bar"));
    connect(closeButton, &QToolButton::clicked, this, &QWidget::hide);
    layout->addWidget(closeButton);

    setLayout(layout);

    setMaximumHeight(sizeHint().height());
    setMinimumWidth(sizeHint().width());
}

void SearchToolBar::setText(const QString &text)
{
    m_lineEdit->setText(text);
}

void SearchToolBar::activate()
{
    show();

    m_lineEdit->selectAll();
    m_lineEdit->setFocus();
}

bool SearchToolBar::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_lineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                findPrevious();
            } else {
                findNext();
            }
            return true;
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            QCoreApplication::sendEvent(m_webView, event);
            return true;
        default:
            break;
        }
    }


    return QWidget::eventFilter(object, event);
}

void SearchToolBar::hideEvent(QHideEvent *event)
{
    hideHighlight();
    m_webView->setFocus();
    QWidget::hideEvent(event);
}

void SearchToolBar::showEvent(QShowEvent *event)
{
    activate();

    QWidget::showEvent(event);
}

void SearchToolBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
    }
}

void SearchToolBar::findNext()
{
    if (!isVisible()) {
        return;
    }

    QWebPage::FindFlags ff = QWebPage::FindWrapsAroundDocument;
#if QT_VERSION >= 0x050700
    ff.setFlag(QWebPage::FindCaseSensitively, m_matchCaseButton->isChecked());
#else
    if (m_matchCaseButton->isChecked()) {
        ff |= QWebPage::FindCaseSensitively;
    }
#endif
    m_webView->findText(m_lineEdit->text(), ff);
}

void SearchToolBar::findPrevious()
{
    if (!isVisible()) {
        return;
    }

    QWebPage::FindFlags ff = QWebPage::FindWrapsAroundDocument;
#if QT_VERSION >= 0x050700
    ff.setFlag(QWebPage::FindCaseSensitively, m_matchCaseButton->isChecked());
    ff.setFlag(QWebPage::FindBackward);
#else
    if (m_matchCaseButton->isChecked()) {
        ff |= QWebPage::FindCaseSensitively;
    }

    ff |= QWebPage::FindBackward;
#endif
    m_webView->findText(m_lineEdit->text(), ff);
}

void SearchToolBar::hideHighlight()
{
    m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
}

void SearchToolBar::updateHighlight()
{
    hideHighlight();

    if (m_highlightAllButton->isChecked()) {
        QWebPage::FindFlags ff = QWebPage::HighlightAllOccurrences;
#if QT_VERSION >= 0x050700
        ff.setFlag(QWebPage::FindCaseSensitively, m_matchCaseButton->isChecked());
#else
        if (m_matchCaseButton->isChecked()) {
            ff |= QWebPage::FindCaseSensitively;
        }
#endif
        m_webView->findText(m_lineEdit->text(), ff);
    }
}
