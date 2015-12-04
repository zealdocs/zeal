/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
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
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
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

#include "qxtglobalshortcut_p.h"

#include <QApplication>
#include <QHash>
#include <QMap>

#include <Carbon/Carbon.h>

typedef QPair<uint, uint> Identifier;
static QMap<quint32, EventHotKeyRef> keyRefs;
static QHash<Identifier, quint32> keyIDs;
static quint32 hotKeySerial = 0;
static bool qxt_mac_handler_installed = false;

OSStatus qxt_mac_handle_hot_key(EventHandlerCallRef nextHandler, EventRef event, void *data)
{
    Q_UNUSED(nextHandler);
    Q_UNUSED(data);
    if (GetEventClass(event) == kEventClassKeyboard && GetEventKind(event) == kEventHotKeyPressed) {
        EventHotKeyID keyID;
        GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(keyID), NULL, &keyID);
        Identifier id = keyIDs.key(keyID.id);
        QxtGlobalShortcutPrivate::activateShortcut(id.second, id.first);
    }
    return noErr;
}

bool QxtGlobalShortcutPrivate::nativeEventFilter(const QByteArray & eventType,
                                                 void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
    return false;
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= shiftKey;
    if (modifiers & Qt::ControlModifier)
        native |= cmdKey;
    if (modifiers & Qt::AltModifier)
        native |= optionKey;
    if (modifiers & Qt::MetaModifier)
        native |= controlKey;
    if (modifiers & Qt::KeypadModifier)
        native |= kEventKeyModifierNumLockMask;
    return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
    UTF16Char ch;
    // Constants found in NSEvent.h from AppKit.framework
    switch (key) {
    case Qt::Key_Return:
        return kVK_Return;
    case Qt::Key_Enter:
        return kVK_ANSI_KeypadEnter;
    case Qt::Key_Tab:
        return kVK_Tab;
    case Qt::Key_Space:
        return kVK_Space;
    case Qt::Key_Backspace:
        return kVK_Delete;
    case Qt::Key_Control:
        return kVK_Command;
    case Qt::Key_Shift:
        return kVK_Shift;
    case Qt::Key_CapsLock:
        return kVK_CapsLock;
    case Qt::Key_Option:
        return kVK_Option;
    case Qt::Key_Meta:
        return kVK_Control;
    case Qt::Key_F17:
        return kVK_F17;
    case Qt::Key_VolumeUp:
        return kVK_VolumeUp;
    case Qt::Key_VolumeDown:
        return kVK_VolumeDown;
    case Qt::Key_F18:
        return kVK_F18;
    case Qt::Key_F19:
        return kVK_F19;
    case Qt::Key_F20:
        return kVK_F20;
    case Qt::Key_F5:
        return kVK_F5;
    case Qt::Key_F6:
        return kVK_F6;
    case Qt::Key_F7:
        return kVK_F7;
    case Qt::Key_F3:
        return kVK_F3;
    case Qt::Key_F8:
        return kVK_F8;
    case Qt::Key_F9:
        return kVK_F9;
    case Qt::Key_F11:
        return kVK_F11;
    case Qt::Key_F13:
        return kVK_F13;
    case Qt::Key_F16:
        return kVK_F16;
    case Qt::Key_F14:
        return kVK_F14;
    case Qt::Key_F10:
        return kVK_F10;
    case Qt::Key_F12:
        return kVK_F12;
    case Qt::Key_F15:
        return kVK_F15;
    case Qt::Key_Help:
        return kVK_Help;
    case Qt::Key_Home:
        return kVK_Home;
    case Qt::Key_PageUp:
        return kVK_PageUp;
    case Qt::Key_Delete:
        return kVK_ForwardDelete;
    case Qt::Key_F4:
        return kVK_F4;
    case Qt::Key_End:
        return kVK_End;
    case Qt::Key_F2:
        return kVK_F2;
    case Qt::Key_PageDown:
        return kVK_PageDown;
    case Qt::Key_F1:
        return kVK_F1;
    case Qt::Key_Left:
        return kVK_LeftArrow;
    case Qt::Key_Right:
        return kVK_RightArrow;
    case Qt::Key_Down:
        return kVK_DownArrow;
    case Qt::Key_Up:
        return kVK_UpArrow;
    default:
        ;
    }

    if (key == Qt::Key_Escape)
        ch = 27;
    else if (key == Qt::Key_Return)
        ch = 13;
    else if (key == Qt::Key_Enter)
        ch = 3;
    else if (key == Qt::Key_Tab)
        ch = 9;
    else
        ch = key;

    CFDataRef currentLayoutData;
    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();

    if (currentKeyboard == NULL)
        return 0;

    currentLayoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(currentKeyboard);
    if (currentLayoutData == NULL)
        return 0;

    UCKeyboardLayout *header = (UCKeyboardLayout *)CFDataGetBytePtr(currentLayoutData);
    UCKeyboardTypeHeader *table = header->keyboardTypeList;

    uint8_t *data = (uint8_t*)header;
    // God, would a little documentation for this shit kill you...
    for (quint32 i = 0; i < header->keyboardTypeCount; ++i) {
        UCKeyStateRecordsIndex *stateRec = 0;
        if (table[i].keyStateRecordsIndexOffset != 0) {
            stateRec = reinterpret_cast<UCKeyStateRecordsIndex *>(data + table[i].keyStateRecordsIndexOffset);
            if (stateRec->keyStateRecordsIndexFormat != kUCKeyStateRecordsIndexFormat) stateRec = 0;
        }

        UCKeyToCharTableIndex *charTable = reinterpret_cast<UCKeyToCharTableIndex *>(data + table[i].keyToCharTableIndexOffset);
        if (charTable->keyToCharTableIndexFormat != kUCKeyToCharTableIndexFormat)
            continue;

        for (quint32 j=0; j < charTable->keyToCharTableCount; ++j) {
            UCKeyOutput *keyToChar = reinterpret_cast<UCKeyOutput *>(data + charTable->keyToCharTableOffsets[j]);
            for (quint32 k=0; k < charTable->keyToCharTableSize; ++k) {
                if (keyToChar[k] & kUCKeyOutputTestForIndexMask) {
                    long idx = keyToChar[k] & kUCKeyOutputGetIndexMask;
                    if (stateRec && idx < stateRec->keyStateRecordCount) {
                        UCKeyStateRecord *rec = reinterpret_cast<UCKeyStateRecord *>(data + stateRec->keyStateRecordOffsets[idx]);
                        if (rec->stateZeroCharData == ch) return k;
                    }
                } else if (!(keyToChar[k] & kUCKeyOutputSequenceIndexMask) && keyToChar[k] < 0xFFFE) {
                    if (keyToChar[k] == ch)
                        return k;
                }
            } // for k
        } // for j
    } // for i
    return 0;
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    if (!qxt_mac_handler_installed) {
        EventTypeSpec t;
        t.eventClass = kEventClassKeyboard;
        t.eventKind = kEventHotKeyPressed;
        InstallApplicationEventHandler(&qxt_mac_handle_hot_key, 1, &t, NULL, NULL);
    }

    EventHotKeyID keyID;
    keyID.signature = 'cute';
    keyID.id = ++hotKeySerial;

    EventHotKeyRef ref = 0;
    bool rv = !RegisterEventHotKey(nativeKey, nativeMods, keyID, GetApplicationEventTarget(), 0, &ref);
    if (rv) {
        keyIDs.insert(Identifier(nativeMods, nativeKey), keyID.id);
        keyRefs.insert(keyID.id, ref);
    }
    return rv;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
    Identifier id(nativeMods, nativeKey);
    if (!keyIDs.contains(id))
        return false;

    EventHotKeyRef ref = keyRefs.take(keyIDs[id]);
    keyIDs.remove(id);
    return !UnregisterEventHotKey(ref);
}
