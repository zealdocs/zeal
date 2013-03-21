#include "zealnativeeventfilter.h"

#ifdef WIN32
#include <windows.h>
#else
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <QGuiApplication>
#include "xcb_keysym.h"
#include <qpa/qplatformnativeinterface.h>
#include <QDebug>

// http://svn.tribler.org/vlc/trunk/modules/control/globalhotkeys/xcb.c
// Copyright (C) 2009 the VideoLAN team
static unsigned GetModifier( xcb_connection_t *p_connection, xcb_key_symbols_t *p_symbols, xcb_keysym_t sym )
{
    static const unsigned pi_mask[8] = {
        XCB_MOD_MASK_SHIFT, XCB_MOD_MASK_LOCK, XCB_MOD_MASK_CONTROL,
        XCB_MOD_MASK_1, XCB_MOD_MASK_2, XCB_MOD_MASK_3,
        XCB_MOD_MASK_4, XCB_MOD_MASK_5
    };

    if( sym == 0 )
        return 0; /* no modifier */

#ifdef XCB_KEYSYM_OLD_API /* as seen in Debian Lenny */
    const xcb_keycode_t key = xcb_key_symbols_get_keycode( p_symbols, sym );
    if( key == 0 )
        return 0;
#else
    const xcb_keycode_t *p_keys = xcb_key_symbols_get_keycode( p_symbols, sym );
    if( !p_keys )
        return 0;

    int i = 0;
    bool no_modifier = true;
    while( p_keys[i] != XCB_NO_SYMBOL )
    {
        if( p_keys[i] != 0 )
        {
            no_modifier = false;
            break;
        }
        i++;
    }

    if( no_modifier )
        return 0;
#endif

    xcb_get_modifier_mapping_cookie_t r =
            xcb_get_modifier_mapping( p_connection );
    xcb_get_modifier_mapping_reply_t *p_map =
            xcb_get_modifier_mapping_reply( p_connection, r, NULL );
    if( !p_map )
        return 0;

    xcb_keycode_t *p_keycode = xcb_get_modifier_mapping_keycodes( p_map );
    if( !p_keycode )
        return 0;

    for( int i = 0; i < 8; i++ )
        for( int j = 0; j < p_map->keycodes_per_modifier; j++ )
#ifdef XCB_KEYSYM_OLD_API /* as seen in Debian Lenny */
            if( p_keycode[i * p_map->keycodes_per_modifier + j] == key )
            {
                free( p_map );
                return pi_mask[i];
            }
#else
            for( int k = 0; p_keys[k] != XCB_NO_SYMBOL; k++ )
                if( p_keycode[i*p_map->keycodes_per_modifier + j] == p_keys[k])
                {
                    free( p_map );
                    return pi_mask[i];
                }
#endif

    free( p_map ); // FIXME to check
    return 0;
}
#endif // WIN32

ZealNativeEventFilter::ZealNativeEventFilter(QObject *parent) :
    QObject(parent), QAbstractNativeEventFilter()
{
}

bool ZealNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
#ifdef WIN32
    MSG* msg = static_cast<MSG*>(message);

    if(WM_HOTKEY == msg->message && msg->wParam == 10) {
        emit gotHotKey();
        return true;
    }
#else // WIN32
    xcb_generic_event_t* ev = static_cast<xcb_generic_event_t*>(message);
    if((ev->response_type&127) == XCB_KEY_PRESS && !hotKey.isEmpty()) {
        xcb_connection_t  *c = static_cast<xcb_connection_t*>(
              ((QGuiApplication*)QGuiApplication::instance())->
                    platformNativeInterface()->nativeResourceForWindow("connection", 0));
        xcb_key_press_event_t *event = (xcb_key_press_event_t *)ev;

        xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(c);
        xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, GetX11Key(hotKey[hotKey.count()-1]));
        int i = 0;
        bool found = false;
        while(keycodes[i] != XCB_NO_SYMBOL) {
            if(event->detail == keycodes[i]) {
                bool modifiers_present = true;
                if(hotKey[hotKey.count()-1] & Qt::ALT) {
                    if(!(event->state & GetModifier(c, keysyms, XK_Alt_L) || event->state & GetModifier(c, keysyms,  XK_Alt_R))) {
                        modifiers_present = false;
                    }
                }
                if(hotKey[hotKey.count()-1] & Qt::CTRL) {
                    if(!(event->state & GetModifier(c, keysyms, XK_Control_L) || event->state & GetModifier(c, keysyms,  XK_Control_R))) {
                        modifiers_present = false;
                    }
                }
                if(hotKey[hotKey.count()-1] & Qt::META) {
                    if(!(event->state & GetModifier(c, keysyms, XK_Meta_L) || event->state & GetModifier(c, keysyms,  XK_Meta_R))) {
                        modifiers_present = false;
                    }
                }
                if(hotKey[hotKey.count()-1] & Qt::SHIFT) {
                    if(!(event->state & GetModifier(c, keysyms, XK_Shift_L) || event->state & GetModifier(c, keysyms,  XK_Shift_R))) {
                        modifiers_present = false;
                    }
                }
                if(enabled && modifiers_present) {
                    xcb_allow_events(c, XCB_ALLOW_ASYNC_KEYBOARD, event->time);
                    emit gotHotKey();
                    found = true;
                } else {
                    xcb_allow_events(c, XCB_ALLOW_REPLAY_KEYBOARD, event->time);
                }
                break;
            }
            i += 1;
        }
        free(keysyms);
        free(keycodes);
        if(found) return true;
    }
#endif // WIN32
    return false;
}
