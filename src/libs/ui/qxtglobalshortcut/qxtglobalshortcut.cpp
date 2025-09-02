// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later
/****************************************************************************
// Copyright (C) 2006 - 2011, the LibQxt project.
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

#include "qxtglobalshortcut.h"
#include "qxtglobalshortcut_p.h"

#include <QAbstractEventDispatcher>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(log, "zeal.ui.qxtglobalshortcut")

#ifndef Q_OS_MACOS
int QxtGlobalShortcutPrivate::ref = 0;
#endif // Q_OS_MACOS

QHash<QPair<quint32, quint32>, QxtGlobalShortcut *> QxtGlobalShortcutPrivate::shortcuts;

QxtGlobalShortcutPrivate::QxtGlobalShortcutPrivate(QxtGlobalShortcut *qq)
    : q_ptr(qq)
{
#ifndef Q_OS_MACOS
    if (ref == 0)
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
    ++ref;
#endif // Q_OS_MACOS
}

QxtGlobalShortcutPrivate::~QxtGlobalShortcutPrivate()
{
#ifndef Q_OS_MACOS
    --ref;
    if (ref == 0) {
        QAbstractEventDispatcher *ed = QAbstractEventDispatcher::instance();
        if (ed != nullptr) {
            ed->removeNativeEventFilter(this);
        }
    }
#endif // Q_OS_MACOS
}

bool QxtGlobalShortcutPrivate::setShortcut(const QKeySequence &shortcut)
{
    Q_Q(QxtGlobalShortcut);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int combination = shortcut[0];
#else
    const int combination = shortcut[0].toCombined();
#endif

    key = shortcut.isEmpty() ? Qt::Key(0) : Qt::Key(combination & ~Qt::KeyboardModifierMask);
    mods = shortcut.isEmpty() ? Qt::NoModifier : Qt::KeyboardModifiers(combination & Qt::KeyboardModifierMask);
    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(mods);
    const bool res = registerShortcut(nativeKey, nativeMods);
    if (!res) {
        qCWarning(log, "Failed to register '%s' shortcut.", qPrintable(QKeySequence(key | mods).toString()));
        return false;
    }

    shortcuts.insert({nativeKey, nativeMods}, q);

    return true;
}

bool QxtGlobalShortcutPrivate::unsetShortcut()
{
    Q_Q(QxtGlobalShortcut);

    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(mods);

    if (shortcuts.value({nativeKey, nativeMods}) != q) {
        qCWarning(log, "Tried to unregister unowned '%s' shortcut.", qPrintable(QKeySequence(key | mods).toString()));
        return false;
    }

    const bool res = unregisterShortcut(nativeKey, nativeMods);
    if (!res) {
        qCWarning(log, "Failed to unregister '%s' shortcut.", qPrintable(QKeySequence(key | mods).toString()));
        return false;
    }

    shortcuts.remove({nativeKey, nativeMods});

    key = Qt::Key(0);
    mods = Qt::KeyboardModifiers(Qt::NoModifier);

    return true;
}

bool QxtGlobalShortcutPrivate::activateShortcut(quint32 nativeKey, quint32 nativeMods)
{
    QxtGlobalShortcut *shortcut = shortcuts.value({nativeKey, nativeMods});
    if (!shortcut || !shortcut->isEnabled())
        return false;

    emit shortcut->activated();
    return true;
}

/*!
    \class QxtGlobalShortcut
    \inmodule QxtWidgets
    \brief The QxtGlobalShortcut class provides a global shortcut aka "hotkey".

    A global shortcut triggers even if the application is not active. This
    makes it easy to implement applications that react to certain shortcuts
    still if some other application is active or if the application is for
    example minimized to the system tray.

    Example usage:
    \code
    QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(window);
    connect(shortcut, SIGNAL(activated()), window, SLOT(toggleVisibility()));
    shortcut->setShortcut(QKeySequence("Ctrl+Shift+F12"));
    \endcode

    \bold {Note:} Since Qxt 0.6 QxtGlobalShortcut no more requires QxtApplication.
 */

/*!
    \fn QxtGlobalShortcut::activated()

    This signal is emitted when the user types the shortcut's key sequence.

    \sa shortcut
 */

/*!
    Constructs a new QxtGlobalShortcut with \a parent.
 */
QxtGlobalShortcut::QxtGlobalShortcut(QObject *parent)
    : QObject(parent)
    , d_ptr(new QxtGlobalShortcutPrivate(this))
{
}

/*!
    Constructs a new QxtGlobalShortcut with \a shortcut and \a parent.
 */
QxtGlobalShortcut::QxtGlobalShortcut(const QKeySequence &shortcut, QObject *parent)
    : QObject(parent)
    , d_ptr(new QxtGlobalShortcutPrivate(this))
{
    setShortcut(shortcut);
}

/*!
    Destructs the QxtGlobalShortcut.
 */
QxtGlobalShortcut::~QxtGlobalShortcut()
{
    Q_D(QxtGlobalShortcut);
    if (d->key != 0)
        d->unsetShortcut();
    delete d;
}

/*!
    \property QxtGlobalShortcut::shortcut
    \brief the shortcut key sequence

    \bold {Note:} Notice that corresponding key press and release events are not
    delivered for registered global shortcuts even if they are disabled.
    Also, comma separated key sequences are not supported.
    Only the first part is used:

    \code
    qxtShortcut->setShortcut(QKeySequence("Ctrl+Alt+A,Ctrl+Alt+B"));
    Q_ASSERT(qxtShortcut->shortcut() == QKeySequence("Ctrl+Alt+A"));
    \endcode
 */
QKeySequence QxtGlobalShortcut::shortcut() const
{
    Q_D(const QxtGlobalShortcut);
    return QKeySequence(d->key | d->mods);
}

bool QxtGlobalShortcut::setShortcut(const QKeySequence &shortcut)
{
    Q_D(QxtGlobalShortcut);
    if (d->key != 0 && !d->unsetShortcut())
        return false;
    if (shortcut.isEmpty())
        return true;
    return d->setShortcut(shortcut);
}

/*!
    \property QxtGlobalShortcut::enabled
    \brief whether the shortcut is enabled

    A disabled shortcut does not get activated.

    The default value is \c true.

    \sa setDisabled()
 */
bool QxtGlobalShortcut::isEnabled() const
{
    Q_D(const QxtGlobalShortcut);
    return d->enabled;
}

/*!
 * \brief QxtGlobalShortcut::isSupported checks if the current platform is supported.
 */
bool QxtGlobalShortcut::isSupported()
{
    return QxtGlobalShortcutPrivate::isSupported();
}

void QxtGlobalShortcut::setEnabled(bool enabled)
{
    Q_D(QxtGlobalShortcut);
    d->enabled = enabled;
}
