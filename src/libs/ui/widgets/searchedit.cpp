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

#include "searchedit.h"

#include <registry/searchquery.h>

#include <QCompleter>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QTreeView>

using namespace Zeal;
using namespace Zeal::WidgetUi;

SearchEdit::SearchEdit(QWidget *parent) :
    QLineEdit(parent)
{
    m_completionLabel = new QLabel(this);
    m_completionLabel->setObjectName(QStringLiteral("completer"));
    m_completionLabel->setStyleSheet(QStringLiteral("QLabel#completer { color: gray; }"));
    m_completionLabel->setFont(font());

    connect(this, &SearchEdit::textChanged, this, &SearchEdit::showCompletions);
}

void SearchEdit::setTreeView(QTreeView *view)
{
    m_treeView = view;
    m_focusing = false;
}

// Makes the line edit use autocompletions.
void SearchEdit::setCompletions(const QStringList &completions)
{
    if (m_prefixCompleter)
        delete m_prefixCompleter;

    m_prefixCompleter = new QCompleter(completions, this);
    m_prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    m_prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_prefixCompleter->setWidget(this);
}

// Clear input with consideration to docset filters
void SearchEdit::clearQuery()
{
    setText(text().left(queryStart()));
}

void SearchEdit::selectQuery()
{
    setSelection(queryStart(), text().size());
}

bool SearchEdit::event(QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QLineEdit::event(event);

    // Tab key cannot be overriden in keyPressEvent()
    if (static_cast<QKeyEvent *>(event)->key() != Qt::Key_Tab)
        return QLineEdit::event(event);

    const QString completed = currentCompletion(text());
    if (!completed.isEmpty())
        setText(completed);

    return true;
}

void SearchEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);

    // Do not change default behaviour when focused with mouse
    if (event->reason() == Qt::MouseFocusReason)
        return;

    selectQuery();
    m_focusing = true;
}

void SearchEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        clearQuery();
        event->accept();
        break;
    case Qt::Key_Return:
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
        QCoreApplication::sendEvent(m_treeView, event);
        break;
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void SearchEdit::mousePressEvent(QMouseEvent *event)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (!m_focusing)
        QLineEdit::mousePressEvent(event);

    m_focusing = false;
}

void SearchEdit::showCompletions(const QString &newValue)
{
    const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    const int textWidth = fontMetrics().width(newValue);

    if (m_prefixCompleter)
        m_prefixCompleter->setCompletionPrefix(text());

    const QString completed = currentCompletion(newValue).mid(newValue.size());
    const QSize labelSize(fontMetrics().width(completed), size().height());

    m_completionLabel->setMinimumSize(labelSize);
    m_completionLabel->move(frameWidth + 2 + textWidth, 0);
    m_completionLabel->setText(completed);
}

QString SearchEdit::currentCompletion(const QString &text) const
{
    if (text.isEmpty() || !m_prefixCompleter)
        return QString();

    return m_prefixCompleter->currentCompletion();
}

int SearchEdit::queryStart() const
{
    const Registry::SearchQuery currentQuery = Registry::SearchQuery::fromString(text());
    // Keep the filter for the first Escape press
    return currentQuery.query().isEmpty() ? 0 : currentQuery.keywordPrefixSize();
}
