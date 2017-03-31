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

#include "shortcutedit.h"

#include <QEvent>
#include <QKeyEvent>

using namespace Zeal::WidgetUi;

ShortcutEdit::ShortcutEdit(QWidget *parent) :
    ShortcutEdit(QString(), parent)
{
}

ShortcutEdit::ShortcutEdit(const QString &text, QWidget *parent) :
    QLineEdit(text, parent)
{
    connect(this, &QLineEdit::textChanged, [this](const QString &text) {
        m_key = QKeySequence(text, QKeySequence::NativeText)[0];
    });
}

bool ShortcutEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
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
