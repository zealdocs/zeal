DEFINES += BUILD_QXT_GUI

INCLUDEPATH += $$PWD

HEADERS += $$files($$PWD/*h)

SOURCES += $$PWD/qxtglobalshortcut.cpp
macx:SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
unix:SOURCES += $$PWD/qxtglobalshortcut_x11.cpp
win32:SOURCES += $$PWD/qxtglobalshortcut_win.cpp

