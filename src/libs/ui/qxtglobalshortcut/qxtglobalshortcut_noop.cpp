// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qxtglobalshortcut_p.h"

bool QxtGlobalShortcutPrivate::isSupported()
{
    return false;
}

bool QxtGlobalShortcutPrivate::nativeEventFilter(
    [[maybe_unused]] const QByteArray &eventType,
    [[maybe_unused]] void *message,
    [[maybe_unused]] NativeEventFilterResult *result)
{
    return false;
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers([[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    return 0;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode([[maybe_unused]] Qt::Key key)
{
    return 0;
}

bool QxtGlobalShortcutPrivate::registerShortcut(
    [[maybe_unused]] quint32 nativeKey,
    [[maybe_unused]] quint32 nativeMods)
{
    return false;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(
    [[maybe_unused]] quint32 nativeKey,
    [[maybe_unused]] quint32 nativeMods)
{
    return false;
}
