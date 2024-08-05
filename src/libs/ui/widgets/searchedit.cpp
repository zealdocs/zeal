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
#include <QKeyEvent>
#include <QLabel>
#include <QStyle>
#include <QTimer>

using namespace Zeal;
using namespace Zeal::WidgetUi;

SearchEdit::SearchEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setClearButtonEnabled(true);
    setPlaceholderText(tr("Search"));

    m_completionLabel = new QLabel(this);
    m_completionLabel->setObjectName(QStringLiteral("completer"));
    m_completionLabel->setStyleSheet(QStringLiteral("QLabel#completer { color: gray; }"));
    m_completionLabel->setFont(font());

    connect(this, &SearchEdit::textChanged, this, &SearchEdit::showCompletions);
}

// Makes the line edit use autocompletion.
void SearchEdit::setCompletions(const QStringList &completions)
{
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
    if (text().isEmpty())
        return;

    const int pos = hasSelectedText() ? selectionStart() : cursorPosition();
    const int queryPos = queryStart();
    const int textSize = text().size();
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    if (pos >= queryPos && selectionEnd() < textSize) {
#else
    const int selectionEnd = hasSelectedText() ? pos + selectedText().size() : -1;
    if (pos >= queryPos && selectionEnd < textSize) {
#endif
        setSelection(queryPos, textSize);
        return;
    }

    // Avoid some race condition which breaks Ctrl+K shortcut.
    QTimer::singleShot(0, this, &QLineEdit::selectAll);
}

bool SearchEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        // Tab key cannot be overriden in keyPressEvent().
        if (keyEvent->key() == Qt::Key_Tab) {
            const QString completed = currentCompletion(text());
            if (!completed.isEmpty()) {
                setText(completed);
            }

            return true;
        } else if (keyEvent->key() == Qt::Key_Escape) {
            clearQuery();
            return true;
        }

        break;
    }
    case QEvent::ShortcutOverride: {
        // TODO: Should be obtained from the ActionManager.
        static const QStringList focusShortcuts = {
            QStringLiteral("Ctrl+K"),
            QStringLiteral("Ctrl+L")
        };

        auto keyEvent = static_cast<QKeyEvent *>(event);
        const int keyCode = keyEvent->key() | static_cast<int>(keyEvent->modifiers());
        if (focusShortcuts.contains(QKeySequence(keyCode).toString())) {
            selectQuery();
            event->accept();
            return true;
        }

        break;
    }
    default:
        break;
    }

    return QLineEdit::event(event);
}

void SearchEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);

    // Do not change the default behaviour when focused with mouse.
    if (event->reason() == Qt::MouseFocusReason)
        return;

    selectQuery();
}

void SearchEdit::showCompletions(const QString &newValue)
{
    if (!isVisible())
        return;

    const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    const int textWidth = fontMetrics().horizontalAdvance(newValue);
#else
    const int textWidth = fontMetrics().width(newValue);
#endif

    if (m_prefixCompleter)
        m_prefixCompleter->setCompletionPrefix(text());

    const QString completed = currentCompletion(newValue).mid(newValue.size());
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    const QSize labelSize(fontMetrics().horizontalAdvance(completed), size().height());
#else
    const QSize labelSize(fontMetrics().width(completed), size().height());
#endif
    const int shiftX = static_cast<int>(window()->devicePixelRatioF() * (frameWidth + 2)) + textWidth;

    m_completionLabel->setMinimumSize(labelSize);
    m_completionLabel->move(shiftX, 0);
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
