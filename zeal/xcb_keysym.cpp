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
