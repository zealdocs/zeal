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

#include "qxtglobalshortcut_p.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QScopedPointer>
#include <QtGui/private/qtx11extras_p.h>
#include <QtGui/private/qxkbcommon_p.h>

// X11 headers define KeyPress/KeyRelease as macros that conflict with Qt enums.
#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

namespace {
constexpr quint32 maskModifiers[] = {0, XCB_MOD_MASK_2, XCB_MOD_MASK_LOCK, (XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK)};
} // namespace

bool QxtGlobalShortcutPrivate::isSupported()
{
    return QGuiApplication::platformName() == QLatin1String("xcb");
}

bool QxtGlobalShortcutPrivate::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)
    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    auto event = static_cast<xcb_generic_event_t *>(message);
    if ((event->response_type & ~0x80) != XCB_KEY_PRESS) {
        return false;
    }

    auto keyPressEvent = static_cast<xcb_key_press_event_t *>(message);

    // Avoid keyboard freeze
    xcb_connection_t *xcbConnection = QX11Info::connection();
    xcb_allow_events(xcbConnection, XCB_ALLOW_REPLAY_KEYBOARD, keyPressEvent->time);
    xcb_flush(xcbConnection);

    unsigned int keycode = keyPressEvent->detail;
    unsigned int keystate = 0;
    if (keyPressEvent->state & XCB_MOD_MASK_1) {
        keystate |= XCB_MOD_MASK_1;
    }
    if (keyPressEvent->state & XCB_MOD_MASK_CONTROL) {
        keystate |= XCB_MOD_MASK_CONTROL;
    }
    if (keyPressEvent->state & XCB_MOD_MASK_4) {
        keystate |= XCB_MOD_MASK_4;
    }
    if (keyPressEvent->state & XCB_MOD_MASK_SHIFT) {
        keystate |= XCB_MOD_MASK_SHIFT;
    }

    return activateShortcut(keycode, keystate);
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier) {
        native |= XCB_MOD_MASK_SHIFT;
    }
    if (modifiers & Qt::ControlModifier) {
        native |= XCB_MOD_MASK_CONTROL;
    }
    if (modifiers & Qt::AltModifier) {
        native |= XCB_MOD_MASK_1;
    }
    if (modifiers & Qt::MetaModifier) {
        native |= XCB_MOD_MASK_4;
    }

    return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key, quint32 &extraNativeMods)
{
    extraNativeMods = 0;
    quint32 native = 0;

    // Use Qt's complete internal keysym table (covers media, launch, and all special keys).
    QKeyEvent dummy(QEvent::KeyPress, key, Qt::NoModifier);
    const auto keysyms = QXkbCommon::toKeysym(&dummy);

    KeySym keysym = keysyms.isEmpty() ? XCB_NO_SYMBOL : keysyms.first();
    if (keysym == XCB_NO_SYMBOL) {
        keysym = static_cast<ushort>(key);
    }

    xcb_connection_t *conn = QX11Info::connection();
    xcb_key_symbols_t *xcbKeySymbols = xcb_key_symbols_alloc(conn);

    QScopedPointer<xcb_keycode_t, QScopedPointerPodDeleter> keycodes(
        xcb_key_symbols_get_keycode(xcbKeySymbols, keysym));

    // Silently returns 0 on failure; registration will fail downstream.
    if (!keycodes.isNull()) {
        native = keycodes.data()[0]; // Use the first keycode

        // Check if Shift is needed to produce this keysym.
        // If the keysym at group 0, level 0 (no modifiers) differs from
        // our target, but group 0, level 1 (Shift) matches, then Shift
        // is an implicit modifier.
        KeySym unshifted = xcb_key_symbols_get_keysym(xcbKeySymbols, native, 0);
        if (unshifted != keysym) {
            KeySym shifted = xcb_key_symbols_get_keysym(xcbKeySymbols, native, 1);
            if (shifted == keysym) {
                extraNativeMods |= XCB_MOD_MASK_SHIFT;
            }
        }
    }

    xcb_key_symbols_free(xcbKeySymbols);

    return native;
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    if (nativeKey == 0) {
        return false;
    }

    xcb_connection_t *xcbConnection = QX11Info::connection();

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcbCookies << xcb_grab_key_checked(xcbConnection,
                                           1,
                                           QX11Info::appRootWindow(),
                                           nativeMods | maskMods,
                                           nativeKey,
                                           XCB_GRAB_MODE_ASYNC,
                                           XCB_GRAB_MODE_ASYNC);
    }

    bool failed = false;
    for (xcb_void_cookie_t cookie : std::as_const(xcbCookies)) {
        QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(xcbConnection, cookie));
        failed |= !error.isNull();
    }

    if (failed) {
        unregisterShortcut(nativeKey, nativeMods);
    }

    return !failed;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
    if (nativeKey == 0) {
        return false;
    }

    xcb_connection_t *xcbConnection = QX11Info::connection();

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcbCookies << xcb_ungrab_key_checked(xcbConnection,
                                             nativeKey,
                                             QX11Info::appRootWindow(),
                                             nativeMods | maskMods);
    }

    bool failed = false;
    for (xcb_void_cookie_t cookie : std::as_const(xcbCookies)) {
        QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(xcbConnection, cookie));
        failed |= !error.isNull();
    }

    return !failed;
}
