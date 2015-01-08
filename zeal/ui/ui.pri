HEADERS += \
    $$files($$PWD/*.h) \
    $$files($$PWD/widgets/*.h)

SOURCES += \
    $$files($$PWD/*.cpp) \
    $$files($$PWD/widgets/*.cpp)

unix:!macx {
    LIBS += -lxcb -lxcb-keysyms
    QMAKE_DEL_DIR = rmdir --ignore-fail-on-non-empty

    !no_libappindicator {
        CONFIG += link_pkgconfig
        PKGCONFIG = gtk+-2.0
        INCLUDEPATH += /usr/include/libappindicator-0.1 \
            /usr/include/gtk-2.0 \
            /usr/lib/gtk-2.0/include
        LIBS += -lappindicator

        DEFINES += USE_LIBAPPINDICATOR
    }
} else {
    HEADERS -= $$PWD/xcb_keysym.h
    SOURCES -= $$PWD/xcb_keysym.cpp
}

FORMS += \
    $$files($$PWD/forms/*.ui)
