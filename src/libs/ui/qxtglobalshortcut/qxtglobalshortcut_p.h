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
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#ifndef QXTGLOBALSHORTCUT_P_H
#define QXTGLOBALSHORTCUT_P_H

#include <QAbstractNativeEventFilter>
#include <QHash>

class QKeySequence;

class QxtGlobalShortcut;

class QxtGlobalShortcutPrivate : public QAbstractNativeEventFilter
{
    QxtGlobalShortcut *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(QxtGlobalShortcut)
public:
    QxtGlobalShortcutPrivate(QxtGlobalShortcut *qq);
    ~QxtGlobalShortcutPrivate() override;

    bool enabled = true;
    Qt::Key key = Qt::Key(0);
    Qt::KeyboardModifiers mods = Qt::NoModifier;

#ifndef Q_OS_MACOS
    static int ref;
#endif // Q_OS_MACOS

    bool setShortcut(const QKeySequence &shortcut);
    bool unsetShortcut();

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message,
                                   long *result) override;

    static bool activateShortcut(quint32 nativeKey, quint32 nativeMods);

private:
    static quint32 nativeKeycode(Qt::Key keycode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    static bool registerShortcut(quint32 nativeKey, quint32 nativeMods);
    static bool unregisterShortcut(quint32 nativeKey, quint32 nativeMods);

    static QHash<QPair<quint32, quint32>, QxtGlobalShortcut *> shortcuts;
};

#endif // QXTGLOBALSHORTCUT_P_H
