/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

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
        default:
            m_key = keyEvent->key();
            m_key |= translateModifiers(keyEvent->modifiers(), keyEvent->text());
            setText(keySequence().toString(QKeySequence::NativeText));
        }

        return true;
    }
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

// Inspired by QKeySequenceEditPrivate::translateModifiers()
int ShortcutEdit::translateModifiers(Qt::KeyboardModifiers state, const QString &text)
{
    int modifiers = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key
    if ((state & Qt::ShiftModifier)
            && (text.isEmpty() || !text.at(0).isPrint()
                || text.at(0).isLetterOrNumber() || text.at(0).isSpace())) {
        modifiers |= Qt::ShiftModifier;
    }
    modifiers |= state & Qt::ControlModifier;
    modifiers |= state & Qt::MetaModifier;
    modifiers |= state & Qt::AltModifier;
    return modifiers;
}
