#include "xcb_keysym.h"
#include <Qt>

const x11_to_qt x11keys_to_qtkeys[] =
{
    { XK_BackSpace,     Qt::Key_Backspace, },
    { XK_Tab,           Qt::Key_Tab, },
    { XK_Return,        Qt::Key_Enter, },
    { XK_Escape,        Qt::Key_Escape, },
    { XK_Home,          Qt::Key_Home, },
    { XK_Left,          Qt::Key_Left, },
    { XK_Up,            Qt::Key_Up, },
    { XK_Right,         Qt::Key_Right, },
    { XK_Down,          Qt::Key_Down, },
    { XK_Page_Up,       Qt::Key_PageUp, },
    { XK_Page_Down,     Qt::Key_PageDown, },
    { XK_End,           Qt::Key_End, },
    { XK_Begin,         Qt::Key_Home, },
    { XK_Insert,        Qt::Key_Insert, },
    { XK_Menu,          Qt::Key_Menu },

    /* Numeric pad keys - this breaks binding, so commented out for now */
    /* { XK_KP_Space,      ' ', },
    { XK_KP_Tab,        Qt::Key_Tab, },
    { XK_KP_Enter,      Qt::Key_Enter, },
    { XK_KP_F1,         Qt::Key_F1, },
    { XK_KP_F2,         Qt::Key_F2, },
    { XK_KP_F3,         Qt::Key_F3, },
    { XK_KP_F4,         Qt::Key_F4, },
    { XK_KP_Home,       Qt::Key_Home, },
    { XK_KP_Left,       Qt::Key_Left, },
    { XK_KP_Up,         Qt::Key_Up, },
    { XK_KP_Right,      Qt::Key_Right, },
    { XK_KP_Down,       Qt::Key_Down, },
    { XK_KP_Page_Up,    Qt::Key_PageUp, },
    { XK_KP_Page_Down,  Qt::Key_PageDown, },
    { XK_KP_End,        Qt::Key_End, },
    { XK_KP_Begin,      Qt::Key_Home, },
    { XK_KP_Insert,     Qt::Key_Insert, },
    { XK_KP_Delete,     Qt::Key_Delete, },
    { XK_KP_Equal,      Qt::Key_Equal, },
    { XK_KP_Multiply,   Qt::Key_multiply, },
    { XK_KP_Add,        Qt::Key_Plus, },
    { XK_KP_Separator,  Qt::Key_Comma, },
    { XK_KP_Subtract,   Qt::Key_hyphen, },
    { XK_KP_Decimal,    Qt::Key_Comma, },
    { XK_KP_Divide,     Qt::Key_division, },
    { XK_KP_0,          Qt::Key_0, },
    { XK_KP_1,          Qt::Key_1, },
    { XK_KP_2,          Qt::Key_2, },
    { XK_KP_3,          Qt::Key_3, },
    { XK_KP_4,          Qt::Key_4, },
    { XK_KP_5,          Qt::Key_5, },
    { XK_KP_6,          Qt::Key_6, },
    { XK_KP_7,          Qt::Key_7, },
    { XK_KP_8,          Qt::Key_8, },
    { XK_KP_9,          Qt::Key_9, }, */

    { XK_F1,            Qt::Key_F1, },
    { XK_F2,            Qt::Key_F2, },
    { XK_F3,            Qt::Key_F3, },
    { XK_F4,            Qt::Key_F4, },
    { XK_F5,            Qt::Key_F5, },
    { XK_F6,            Qt::Key_F6, },
    { XK_F7,            Qt::Key_F7, },
    { XK_F8,            Qt::Key_F8, },
    { XK_F9,            Qt::Key_F9, },
    { XK_F10,           Qt::Key_F10, },
    { XK_F11,           Qt::Key_F11, },
    { XK_F12,           Qt::Key_F12, },
    { XK_Delete,        Qt::Key_Delete, },

    /* XFree86 extensions */
    { XF86XK_AudioLowerVolume, Qt::Key_VolumeDown, },
    { XF86XK_AudioMute,        Qt::Key_VolumeMute, },
    { XF86XK_AudioRaiseVolume, Qt::Key_VolumeUp, },
    { XF86XK_AudioPlay,        Qt::Key_MediaTogglePlayPause, },
    { XF86XK_AudioStop,        Qt::Key_MediaStop, },
    { XF86XK_AudioPrev,        Qt::Key_MediaPrevious, },
    { XF86XK_AudioNext,        Qt::Key_MediaNext, },
    { XF86XK_HomePage,         Qt::Key_HomePage, },
    { XF86XK_Search,           Qt::Key_Search, },
    { XF86XK_Back,             Qt::Key_Back, },
    { XF86XK_Forward,          Qt::Key_Forward, },
    { XF86XK_Stop,             Qt::Key_Stop, },
    { XF86XK_Refresh,          Qt::Key_Refresh, },
    { XF86XK_Favorites,        Qt::Key_Favorites, },
    { XF86XK_AudioPause,       Qt::Key_MediaTogglePlayPause, },
    { XF86XK_Reload,           Qt::Key_Reload, },
    { 0, 0 }
};

// the code below comes from http://www.videolan.org/developers/vlc/modules/control/globalhotkeys/xcb.c
/*****************************************************************************
 * xcb.c: Global-Hotkey X11 using xcb handling for vlc
 *****************************************************************************
 * Copyright (C) 2009 the VideoLAN team
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

xcb_keysym_t GetX11Key( unsigned i_qt )
{
    i_qt = i_qt & ~(Qt::ALT | Qt::CTRL | Qt::SHIFT | Qt::META);

    /* X11 and Qt use ASCII for printable ASCII characters */
    if( i_qt >= 32 && i_qt <= 127 )
        return i_qt;

    for( int i = 0; x11keys_to_qtkeys[i].i_qt != 0; i++ )
    {
        if( x11keys_to_qtkeys[i].i_qt == i_qt )
            return x11keys_to_qtkeys[i].i_x11;
    }

    return XK_VoidSymbol;
}

static unsigned GetModifier( xcb_connection_t *p_connection, xcb_key_symbols_t *p_symbols, xcb_keysym_t sym )
{
    static const unsigned pi_mask[8] = {
        XCB_MOD_MASK_SHIFT, XCB_MOD_MASK_LOCK, XCB_MOD_MASK_CONTROL,
        XCB_MOD_MASK_1, XCB_MOD_MASK_2, XCB_MOD_MASK_3,
        XCB_MOD_MASK_4, XCB_MOD_MASK_5
    };

    if( sym == 0 )
        return 0; /* no modifier */

    xcb_keycode_t *p_keys = xcb_key_symbols_get_keycode( p_symbols, sym );
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

    if( no_modifier ) {
        free( p_keys );
        return 0;
    }

    xcb_get_modifier_mapping_cookie_t r =
            xcb_get_modifier_mapping( p_connection );
    xcb_get_modifier_mapping_reply_t *p_map =
            xcb_get_modifier_mapping_reply( p_connection, r, NULL );
    if( !p_map ) {
        free( p_keys );
        return 0;
    }

    xcb_keycode_t *p_keycode = xcb_get_modifier_mapping_keycodes( p_map );
    if( !p_keycode ) {
        free( p_keys );
        free( p_map );
        return 0;
    }

    for( int i = 0; i < 8; i++ )
        for( int j = 0; j < p_map->keycodes_per_modifier; j++ )
            for( int k = 0; p_keys[k] != XCB_NO_SYMBOL; k++ )
                if( p_keycode[i*p_map->keycodes_per_modifier + j] == p_keys[k])
                {
                    free( p_keys );
                    free( p_map );
                    return pi_mask[i];
                }

    free( p_keys );
    free( p_map ); // FIXME to check
    return 0;
}


QList<unsigned> GetX11Modifier( xcb_connection_t *p_connection,
        xcb_key_symbols_t *p_symbols, unsigned i_qt )
{
    unsigned i_nlock = GetModifier(p_connection, p_symbols, XK_Num_Lock),
             i_slock = GetModifier(p_connection, p_symbols, XK_Scroll_Lock);
    unsigned i_clock = XCB_MOD_MASK_LOCK;

    unsigned i_mask = 0;

    if( i_qt & Qt::ALT )
        i_mask |= GetModifier( p_connection, p_symbols, XK_Alt_L ) |
                  GetModifier( p_connection, p_symbols, XK_Alt_R );
    if( i_qt & Qt::CTRL )
        i_mask |= GetModifier( p_connection, p_symbols, XK_Control_L ) |
                  GetModifier( p_connection, p_symbols, XK_Control_R );
    if( i_qt & Qt::SHIFT )
        i_mask |= GetModifier( p_connection, p_symbols, XK_Shift_L ) |
                  GetModifier( p_connection, p_symbols, XK_Shift_R );
    if( i_qt & Qt::META )
        i_mask |= GetModifier( p_connection, p_symbols, XK_Meta_L ) |
                  GetModifier( p_connection, p_symbols, XK_Meta_R );

    QList<unsigned> ret;
    ret.append(i_mask);

    if(i_nlock) ret.append(i_mask | i_nlock);
    if(i_slock) ret.append(i_mask | i_slock);
    if(i_clock) ret.append(i_mask | i_clock);

    if(i_nlock && i_slock) ret.append(i_mask | i_nlock | i_slock);
    if(i_nlock && i_clock) ret.append(i_mask | i_nlock | i_clock);
    if(i_slock && i_clock) ret.append(i_mask | i_slock | i_clock);
    if(i_nlock && i_slock && i_clock) ret.append(i_mask | i_nlock | i_clock);

    return ret;
}
