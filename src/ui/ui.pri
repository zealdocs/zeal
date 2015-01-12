HEADERS += \
    $$files($$PWD/*.h) \
    $$files($$PWD/widgets/*.h)

SOURCES += \
    $$files($$PWD/*.cpp) \
    $$files($$PWD/widgets/*.cpp)

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += x11

    QMAKE_DEL_DIR = rmdir --ignore-fail-on-non-empty

    !no_libappindicator {
        PKGCONFIG += gtk+-2.0
        INCLUDEPATH += /usr/include/libappindicator-0.1 \
            /usr/include/gtk-2.0 \
            /usr/lib/gtk-2.0/include
        LIBS += -lappindicator

        DEFINES += USE_LIBAPPINDICATOR
    }
}

FORMS += \
    $$files($$PWD/forms/*.ui)
