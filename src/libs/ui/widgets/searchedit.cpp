// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

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
    m_completionLabel->setFont(font());
    m_completionLabel->setObjectName(QStringLiteral("completer"));
    m_completionLabel->setStyleSheet(QStringLiteral("QLabel#completer { color: gray; }"));

    connect(this, &SearchEdit::textChanged, this, &SearchEdit::showCompletions);
}

// Makes the line edit use autocompletion.
void SearchEdit::setCompletions(const QStringList &completions)
{
    delete m_prefixCompleter;

    m_prefixCompleter = new QCompleter(completions, this);
    m_prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
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
    if (pos >= queryPos && selectionEnd() < textSize) {
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

    // Do not change the default behavior when focused with mouse.
    if (event->reason() == Qt::MouseFocusReason)
        return;

    selectQuery();
}

void SearchEdit::showCompletions(const QString &newValue)
{
    if (!isVisible())
        return;

    const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    const int textWidth = fontMetrics().horizontalAdvance(newValue);

    if (m_prefixCompleter)
        m_prefixCompleter->setCompletionPrefix(text());

    const QString completed = currentCompletion(newValue).mid(newValue.size());
    const QSize labelSize(fontMetrics().horizontalAdvance(completed), size().height());
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
