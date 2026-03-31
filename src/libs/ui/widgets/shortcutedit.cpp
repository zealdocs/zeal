// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shortcutedit.h"

#include <QEvent>
#include <QKeyEvent>

using namespace Zeal::WidgetUi;

ShortcutEdit::ShortcutEdit(QWidget *parent)
    : ShortcutEdit(QString(), parent)
{
}

ShortcutEdit::ShortcutEdit(const QString &text, QWidget *parent)
    : QLineEdit(text, parent)
{
    connect(this, &QLineEdit::textChanged, [this](const QString &text) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_key = QKeySequence(text, QKeySequence::NativeText)[0];
#else
        m_key = QKeySequence(text, QKeySequence::NativeText)[0].toCombined();
#endif
    });
}

bool ShortcutEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_Meta:
        case Qt::Key_Shift:
            return true;
        case Qt::Key_Escape:
            setKeySequence(QKeySequence(m_originalKey));
            clearFocus();
            return true;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            return QLineEdit::event(event);
        default:
            m_key = keyEvent->key();
            m_key |= translateModifiers(keyEvent->modifiers(), keyEvent->text());
            setText(keySequence().toString(QKeySequence::NativeText));
        }

        return true;
    }
    case QEvent::FocusIn:
        m_originalKey = m_key;
        return QLineEdit::event(event);
    case QEvent::ShortcutOverride:
    case QEvent::KeyRelease:
    case QEvent::Shortcut:
        return true;
    default:
        return QLineEdit::event(event);
    }
}

QKeySequence ShortcutEdit::keySequence() const
{
    return QKeySequence(m_key);
}

void ShortcutEdit::setKeySequence(const QKeySequence &keySequence)
{
    setText(keySequence.toString(QKeySequence::NativeText));
}

int ShortcutEdit::translateModifiers(Qt::KeyboardModifiers state, const QString &text)
{
    int modifiers = state & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);

    // Strip Shift when it was used to produce a printable symbol (e.g., Shift+/ = ?),
    // so the shortcut displays as Ctrl+? rather than Ctrl+Shift+?.
    // The actual Shift modifier is recovered during native hotkey registration.
    if (state & Qt::ShiftModifier) {
        const bool isShiftedSymbol = !text.isEmpty() && text.at(0).isPrint() && !text.at(0).isLetterOrNumber()
                                  && !text.at(0).isSpace();
        if (!isShiftedSymbol)
            modifiers |= Qt::ShiftModifier;
    }

    return modifiers;
}
