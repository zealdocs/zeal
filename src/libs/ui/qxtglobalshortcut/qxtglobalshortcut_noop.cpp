// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qxtglobalshortcut_p.h"

bool QxtGlobalShortcutPrivate::isSupported()
{
    return false;
}

bool QxtGlobalShortcutPrivate::nativeEventFilter(const QByteArray &eventType,
                                                 void *message,
                                                 NativeEventFilterResult *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)

    return false;
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers)

    return 0;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
    Q_UNUSED(key)

    return 0;
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    Q_UNUSED(nativeKey)
    Q_UNUSED(nativeMods)

    return false;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
    Q_UNUSED(nativeKey)
    Q_UNUSED(nativeMods)

    return false;
}
