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


#ifndef RAZORSHORTCUTBUTTON_P_H
#define RAZORSHORTCUTBUTTON_P_H

#include "razorshortcutbutton.h"
#include <QMenu>

class QKeyEvent;

class RazorShortcutButtonPrivate: public QObject
{
    Q_OBJECT
public:
    explicit RazorShortcutButtonPrivate(RazorShortcutButton *parent);

    bool keyPressEvent(QKeyEvent *event);
    bool keyReleaseEvent(QKeyEvent *event);

public slots:
    void clear();
    void activate(bool active);

private:
    RazorShortcutButton* const q_ptr;
    Q_DECLARE_PUBLIC(RazorShortcutButton);

    QKeySequence mSequence;
    QMenu mMenu;
    int mKeysCount;
};

#endif // RAZORSHORTCUTBUTTON_P_H
