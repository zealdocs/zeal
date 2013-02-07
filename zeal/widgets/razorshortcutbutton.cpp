/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */


#include "razorshortcutbutton.h"
#include "razorshortcutbutton_p.h"

#include <QtGui/QKeyEvent>
#include <QtCore/QDebug>

/************************************************

 ************************************************/
RazorShortcutButton::RazorShortcutButton(QWidget *parent) :
    QToolButton(parent),
    d_ptr(new RazorShortcutButtonPrivate(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setPopupMode(QToolButton::MenuButtonPopup);
    setCheckable(true);

    Q_D(RazorShortcutButton);
    QAction *a = d->mMenu.addAction("Clear");
    connect(a, SIGNAL(triggered()), d, SLOT(clear()));
    QAction *a_def = d->mMenu.addAction("Default");
    connect(a_def, &QAction::triggered, [=]() {
        setKeySequence(QKeySequence("Alt+Space"));
    });
    setMenu(&d->mMenu);

    connect(this, SIGNAL(toggled(bool)), d, SLOT(activate(bool)));
    setKeySequence("");
}


/************************************************

 ************************************************/
RazorShortcutButtonPrivate::RazorShortcutButtonPrivate(RazorShortcutButton *parent):
    q_ptr(parent),
    mKeysCount(0)
{
}


/************************************************

 ************************************************/
void RazorShortcutButtonPrivate::activate(bool active)
{
    Q_Q(RazorShortcutButton);
    mKeysCount = 0;

    if (active)
        q->grabKeyboard();
    else
        q->releaseKeyboard();

}


/************************************************

 ************************************************/
RazorShortcutButton::~RazorShortcutButton()
{
    releaseKeyboard();
    Q_D(RazorShortcutButton);
    delete d;
}


/************************************************

 ************************************************/
bool RazorShortcutButton::event(QEvent *event)
{
    Q_D(RazorShortcutButton);

    if (isChecked())
    {
        if (event->type() == QEvent::KeyPress)
            return d->keyPressEvent(static_cast<QKeyEvent*>(event));

        if (event->type() == QEvent::KeyRelease)
            return d->keyReleaseEvent(static_cast<QKeyEvent*>(event));

        if (event->type() == QEvent::FocusOut)
            setChecked(false);
    }

    return QToolButton::event(event);
}


/************************************************

 ************************************************/
bool RazorShortcutButtonPrivate::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return true;

    mKeysCount++;
    Q_Q(RazorShortcutButton);

    int key = 0;
    switch (event->key())
    {
    case Qt::Key_Escape:
        return false;
    case Qt::Key_AltGr: //or else we get unicode salad
        return false;
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_Menu:
        break;
    default:
        key = event->key();
        break;
    }

    if (key)
    {
        QKeySequence seq(key + event->modifiers());
        q->setKeySequence(seq);
        return true;
    }

    return false;
}


/************************************************

 ************************************************/
bool RazorShortcutButtonPrivate::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return true;
    Q_Q(RazorShortcutButton);

    mKeysCount--;

    if (mKeysCount<1)
    {
        q->setChecked(false);
    }

    return false;
}


/************************************************

 ************************************************/
QKeySequence RazorShortcutButton::keySequence() const
{
   Q_D(const RazorShortcutButton);
    return d->mSequence;
}



/************************************************

 ************************************************/
void RazorShortcutButton::setKeySequence(const QKeySequence &sequence)
{
    Q_D(RazorShortcutButton);
    d->mSequence = QKeySequence(sequence);

    QString s = d->mSequence.toString();
    if (s.isEmpty())
        setText("None");
    else
        setText(s);

    emit keySequenceChanged(d->mSequence);
    emit keySequenceChanged(s);
}


/************************************************

 ************************************************/
void RazorShortcutButton::setKeySequence(const QString &sequence)
{
    setKeySequence(QKeySequence(sequence));
}


/************************************************

 ************************************************/
void RazorShortcutButtonPrivate::clear()
{
    Q_Q(RazorShortcutButton);
    q->setKeySequence("");
}
