DEFINES += BUILD_QXT_GUI

INCLUDEPATH += $$PWD

HEADERS += $$files($$PWD/*.h)
SOURCES += $$PWD/qxtglobalshortcut.cpp

unix:!macx {
    QT += x11extras

    CONFIG += link_pkgconfig
    PKGCONFIG += x11 xcb

    SOURCES += $$PWD/qxtglobalshortcut_x11.cpp
}

win32:SOURCES += $$PWD/qxtglobalshortcut_win.cpp

macx {
    SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
    LIBS += -framework Carbon
}
