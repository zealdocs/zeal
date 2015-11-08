TEMPLATE = app

QT += gui widgets sql
CONFIG += c++11 silent

# Build features
webengine {
    QT += webenginewidgets
    DEFINES += USE_WEBENGINE
} else {
    QT += webkitwidgets
}

portable {
    DEFINES += PORTABLE_BUILD
}

VERSION = 0.2.0
DEFINES += ZEAL_VERSION=\\\"$${VERSION}\\\"

HEADERS += \
    util/version.h \
    util/plist.h

SOURCES += \
    main.cpp \
    util/version.cpp \
    util/plist.cpp

include(core/core.pri)
include(registry/registry.pri)
include(ui/ui.pri)
include(3rdparty/qxtglobalshortcut/qxtglobalshortcut.pri)

RESOURCES += \
    resources/zeal.qrc

DESTDIR = $$BUILD_ROOT/bin

unix:!macx {
    TARGET = zeal
    isEmpty(PREFIX): PREFIX = /usr
    target.path = $$PREFIX/bin
    INSTALLS = target

    appicons16.files=appicons/16/*
    appicons24.files=appicons/24/*
    appicons32.files=appicons/32/*
    appicons64.files=appicons/64/*
    appicons128.files=appicons/128/*

    appicons16.path=$$PREFIX/share/icons/hicolor/16x16/apps
    appicons24.path=$$PREFIX/share/icons/hicolor/24x24/apps
    appicons32.path=$$PREFIX/share/icons/hicolor/32x32/apps
    appicons64.path=$$PREFIX/share/icons/hicolor/64x64/apps
    appicons128.path=$$PREFIX/share/icons/hicolor/128x128/apps

    desktop.files=zeal.desktop
    desktop.path=$$PREFIX/share/applications

    INSTALLS += appicons16 appicons24 appicons32 appicons64 appicons128 desktop
}

win32 {
    TARGET = zeal
    RC_ICONS = resources/zeal.ico
}

macx {
    TARGET = Zeal
    ICON = resources/zeal.icns
}

# Keep build directory organised
MOC_DIR = $$BUILD_ROOT/.moc
OBJECTS_DIR = $$BUILD_ROOT/.obj
RCC_DIR = $$BUILD_ROOT/.rcc
UI_DIR = $$BUILD_ROOT/.ui
