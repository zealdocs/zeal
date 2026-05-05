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
#include <QKeySequence>
#include <QList>
#include <QScopedPointer>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

namespace {
constexpr quint32 maskModifiers[] = {0, XCB_MOD_MASK_2, XCB_MOD_MASK_LOCK, (XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK)};

namespace X11 {
xcb_connection_t *connection()
{
    auto *iface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return iface ? iface->connection() : nullptr;
}

xcb_window_t rootWindow(xcb_connection_t *xcbConnection)
{
    return xcb_setup_roots_iterator(xcb_get_setup(xcbConnection)).data->root;
}

xcb_keysym_t qtKeyToKeysym(Qt::Key key)
{
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
        return XKB_KEY_F1 + (key - Qt::Key_F1);
    }

    switch (key) {
    case Qt::Key_Escape:
        return XKB_KEY_Escape;
    case Qt::Key_Tab:
        return XKB_KEY_Tab;
    case Qt::Key_Backtab:
        return XKB_KEY_ISO_Left_Tab;
    case Qt::Key_Backspace:
        return XKB_KEY_BackSpace;
    case Qt::Key_Return:
        return XKB_KEY_Return;
    case Qt::Key_Enter:
        return XKB_KEY_KP_Enter;
    case Qt::Key_Insert:
        return XKB_KEY_Insert;
    case Qt::Key_Delete:
        return XKB_KEY_Delete;
    case Qt::Key_Pause:
        return XKB_KEY_Pause;
    case Qt::Key_Print:
        return XKB_KEY_Print;
    case Qt::Key_ScrollLock:
        return XKB_KEY_Scroll_Lock;
    case Qt::Key_Home:
        return XKB_KEY_Home;
    case Qt::Key_End:
        return XKB_KEY_End;
    case Qt::Key_Left:
        return XKB_KEY_Left;
    case Qt::Key_Up:
        return XKB_KEY_Up;
    case Qt::Key_Right:
        return XKB_KEY_Right;
    case Qt::Key_Down:
        return XKB_KEY_Down;
    case Qt::Key_PageUp:
        return XKB_KEY_Page_Up;
    case Qt::Key_PageDown:
        return XKB_KEY_Page_Down;
    case Qt::Key_Menu:
        return XKB_KEY_Menu;

    // Browser keys (XF86 vendor extensions).
    case Qt::Key_Back:
        return XKB_KEY_XF86Back;
    case Qt::Key_Forward:
        return XKB_KEY_XF86Forward;
    case Qt::Key_Refresh:
        return XKB_KEY_XF86Refresh;
    case Qt::Key_Stop:
        return XKB_KEY_XF86Stop;
    case Qt::Key_Reload:
        return XKB_KEY_XF86Reload;
    case Qt::Key_Search:
        return XKB_KEY_XF86Search;
    case Qt::Key_HomePage:
        return XKB_KEY_XF86HomePage;
    case Qt::Key_Favorites:
        return XKB_KEY_XF86Favorites;
    case Qt::Key_OpenUrl:
        return XKB_KEY_XF86OpenURL;

    // Multimedia and system keys (XF86 vendor extensions).
    case Qt::Key_VolumeMute:
        return XKB_KEY_XF86AudioMute;
    case Qt::Key_VolumeDown:
        return XKB_KEY_XF86AudioLowerVolume;
    case Qt::Key_VolumeUp:
        return XKB_KEY_XF86AudioRaiseVolume;
    case Qt::Key_MediaPlay:
        return XKB_KEY_XF86AudioPlay;
    case Qt::Key_MediaStop:
        return XKB_KEY_XF86AudioStop;
    case Qt::Key_MediaPrevious:
        return XKB_KEY_XF86AudioPrev;
    case Qt::Key_MediaNext:
        return XKB_KEY_XF86AudioNext;
    case Qt::Key_MediaPause:
        return XKB_KEY_XF86AudioPause;
    case Qt::Key_MediaTogglePlayPause:
        return XKB_KEY_XF86AudioPlay;
    case Qt::Key_MediaRecord:
        return XKB_KEY_XF86AudioRecord;
    case Qt::Key_MonBrightnessDown:
        return XKB_KEY_XF86MonBrightnessDown;
    case Qt::Key_MonBrightnessUp:
        return XKB_KEY_XF86MonBrightnessUp;

    default:
        // ASCII/Latin-1: legacy keysym == codepoint. Lowercase letters so we
        // match level-0 of the keymap and avoid spurious Shift inference for
        // shortcuts like Ctrl+A.
        if (key < 0x100) {
            return xkb_keysym_to_lower(static_cast<xcb_keysym_t>(key));
        }

        // Non-Latin Unicode codepoint: X11 Unicode keysym encoding.
        if (key < 0x01000000) {
            return key | 0x01000000;
        }

        // Unhandled Qt special key (Help, Cancel, modifiers, etc.). Fail
        // registration rather than passing a special-key encoding through as
        // a Unicode keysym, which would grab the wrong physical key.
        return XCB_NO_SYMBOL;
    }
}
} // namespace X11
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
    xcb_connection_t *xcbConnection = X11::connection();
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

    const xcb_keysym_t keysym = X11::qtKeyToKeysym(key);
    if (keysym == XCB_NO_SYMBOL) {
        return 0;
    }

    xcb_connection_t *xcbConnection = X11::connection();
    if (xcbConnection == nullptr) {
        return 0;
    }

    xcb_key_symbols_t *xcbKeySymbols = xcb_key_symbols_alloc(xcbConnection);
    if (xcbKeySymbols == nullptr) {
        return 0;
    }

    QScopedPointer<xcb_keycode_t, QScopedPointerPodDeleter> keycodes(
        xcb_key_symbols_get_keycode(xcbKeySymbols, keysym));

    // Silently returns 0 on failure; registration will fail downstream.
    if (!keycodes.isNull()) {
        native = keycodes.data()[0]; // Use the first keycode

        // Check if Shift is needed to produce this keysym.
        // If the keysym at group 0, level 0 (no modifiers) differs from
        // our target, but group 0, level 1 (Shift) matches, then Shift
        // is an implicit modifier.
        xcb_keysym_t unshifted = xcb_key_symbols_get_keysym(xcbKeySymbols, native, 0);
        if (unshifted != keysym) {
            xcb_keysym_t shifted = xcb_key_symbols_get_keysym(xcbKeySymbols, native, 1);
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

    xcb_connection_t *xcbConnection = X11::connection();
    if (xcbConnection == nullptr) {
        return false;
    }

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcbCookies << xcb_grab_key_checked(xcbConnection,
                                           1,
                                           X11::rootWindow(xcbConnection),
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

    xcb_connection_t *xcbConnection = X11::connection();
    if (xcbConnection == nullptr) {
        return false;
    }

    QList<xcb_void_cookie_t> xcbCookies;
    for (quint32 maskMods : maskModifiers) {
        xcbCookies << xcb_ungrab_key_checked(xcbConnection,
                                             nativeKey,
                                             X11::rootWindow(xcbConnection),
                                             nativeMods | maskMods);
    }

    bool failed = false;
    for (xcb_void_cookie_t cookie : std::as_const(xcbCookies)) {
        QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(xcbConnection, cookie));
        failed |= !error.isNull();
    }

    return !failed;
}
