#include "mainwindow.h"
#include <QApplication>
#include <QLocalSocket>

#include <string>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>

using namespace std;

void run_grabkey();

int main(int argc, char *argv[])
{
    if(argc > 1 && argv[1] == string("--grabkey")) {
        run_grabkey();
        return -1;  // infinite loop - shouldn't terminate
    } else {
        // detect already running instance
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(500)) {
            cerr << "Already running. Terminating." << endl;
            return -1; // Exit already a process running
        }

        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        return a.exec();
    }
}

void run_grabkey()
{
    Display *display = XOpenDisplay(NULL);

    unsigned int AltMask = 0;

    XModifierKeymap *xmk = XGetModifierMapping(display);
    if(xmk) {
        KeyCode altKeyCode = XKeysymToKeycode(display, XK_Alt_L);
        KeyCode *c = xmk->modifiermap;
        int m, k;
        for(m = 0; m < 8; m++) {
            for(k = 0; k < xmk->max_keypermod; k++, c++)
            {
                if(*c == NoSymbol) {
                    continue;
                }
                if(*c == altKeyCode) {
                    AltMask = (1 << m);
                }
            }
        }
        XFreeModifiermap (xmk);
    }

    if(AltMask == 0) {
        cout << "failed" << endl;
    } else {
        cout << "ok" << endl;
    }

    KeyCode hotKey = XKeysymToKeycode(display, XStringToKeysym("space"));
    XGrabKey(display, hotKey, AnyModifier, DefaultRootWindow(display), True, GrabModeSync, GrabModeSync);

    XEvent e;
    for(;;)
    {
        XNextEvent(display, &e);
        if(e.type == KeyPress){
            if(e.xkey.keycode == hotKey && e.xkey.state & AltMask)
            {
                cout << "1" << endl;
                XAllowEvents(display, AsyncKeyboard, e.xkey.time);
            } else {
                XAllowEvents(display, ReplayKeyboard, e.xkey.time);
                XFlush(display);
            }
        }
    }
}
