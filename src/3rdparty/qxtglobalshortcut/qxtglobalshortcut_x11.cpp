#include "qxtglobalshortcut_p.h"
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

#include <QKeySequence>
#include <QScopedPointer>
#include <QVector>
#include <QX11Info>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

namespace {
const QVector<quint32> maskModifiers = {
    0, XCB_MOD_MASK_2, XCB_MOD_MASK_LOCK, (XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK)
};
} // namespace

bool QxtGlobalShortcutPrivate::nativeEventFilter(const QByteArray &eventType,
                                                 void *message, long *result)
{
    Q_UNUSED(result);
    if (eventType != "xcb_generic_event_t")
        return false;

    xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
    if ((event->response_type & ~0x80) != XCB_KEY_PRESS)
        return false;

    xcb_key_press_event_t *keyPressEvent = reinterpret_cast<xcb_key_press_event_t *>(event);

    // Avoid keyboard freeze
    xcb_connection_t *xcbConnection = QX11Info::connection();
    xcb_allow_events(xcbConnection, XCB_ALLOW_REPLAY_KEYBOARD, keyPressEvent->time);
    xcb_flush(xcbConnection);

    unsigned int keycode = keyPressEvent->detail;
    unsigned int keystate = 0;
    if (keyPressEvent->state & XCB_MOD_MASK_1)
        keystate |= XCB_MOD_MASK_1;
    if (keyPressEvent->state & XCB_MOD_MASK_CONTROL)
        keystate |= XCB_MOD_MASK_CONTROL;
    if (keyPressEvent->state & XCB_MOD_MASK_4)
        keystate |= XCB_MOD_MASK_4;
    if (keyPressEvent->state & XCB_MOD_MASK_SHIFT)
        keystate |= XCB_MOD_MASK_SHIFT;

    return activateShortcut(keycode, keystate);
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= XCB_MOD_MASK_SHIFT;
    if (modifiers & Qt::ControlModifier)
        native |= XCB_MOD_MASK_CONTROL;
    if (modifiers & Qt::AltModifier)
        native |= XCB_MOD_MASK_1;
    if (modifiers & Qt::MetaModifier)
        native |= XCB_MOD_MASK_4;

    return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
    KeySym keysym = XStringToKeysym(QKeySequence(key).toString().toLatin1().data());
    if (keysym == NoSymbol)
        keysym = static_cast<ushort>(key);

    return XKeysymToKeycode(QX11Info::display(), keysym);
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    xcb_connection_t *xcbConnection = QX11Info::connection();

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcbCookies << xcb_grab_key_checked(xcbConnection, 1, QX11Info::appRootWindow(),
                                           nativeMods | maskMods, nativeKey,
                                           XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }

    bool failed = false;
    for (xcb_void_cookie_t cookie : xcbCookies) {
        QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(xcbConnection, cookie));
        failed = !error.isNull();
    }

    if (failed)
        unregisterShortcut(nativeKey, nativeMods);

    return !failed;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
    xcb_connection_t *xcbConnection = QX11Info::connection();

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcb_ungrab_key(xcbConnection, nativeKey, QX11Info::appRootWindow(), nativeMods | maskMods);

    }

    bool failed = false;
    for (xcb_void_cookie_t cookie : xcbCookies) {
        QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(xcbConnection, cookie));
        failed = !error.isNull();
    }

    return !failed;
}
