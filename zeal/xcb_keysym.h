#ifndef XCB_KEYSYM_H
#define XCB_KEYSYM_H

#include <QList>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

typedef struct
{
    xcb_keysym_t i_x11;
    unsigned     i_qt;

} x11_to_qt;

extern const x11_to_qt x11keys_to_qtkeys[];

extern xcb_keysym_t GetX11Key( unsigned i_qt );
extern QList<unsigned> GetX11Modifier( xcb_connection_t *p_connection, xcb_key_symbols_t *p_symbols, unsigned i_qt );

#endif
