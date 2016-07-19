TEMPLATE = app

QT += gui widgets sql concurrent
CONFIG += c++11 silent

## Build options
# Browser engine
CONFIG(zeal_qtwebkit) {
    qtHaveModule(webkitwidgets): BROWSER_ENGINE = qtwebkit
    else: error("Qt WebKit is not available.")
} else:CONFIG(zeal_qtwebengine) {
    qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
    else: error("Qt WebEngine is not available.")
} else {
    qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
    else: qtHaveModule(webkitwidgets): BROWSER_ENGINE = qtwebkit
    else: error("Zeal requires Qt WebEngine or Qt WebKit.")
}

equals(BROWSER_ENGINE, qtwebengine) {
    message("Browser engine: Qt WebEngine.")
    QT += webenginewidgets
    DEFINES += USE_WEBENGINE
} else {
    message("Browser engine: Qt WebKit.")
    QT += webkitwidgets
    DEFINES += USE_WEBKIT
}

# Portable build
CONFIG(zeal_portable) {
    message("Portable build: Yes.")
    DEFINES += PORTABLE_BUILD
} else {
    message("Portable build: No.")
}

VERSION = 0.2.1
DEFINES += ZEAL_VERSION=\\\"$${VERSION}\\\"

# QString options
DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES *= QT_RESTRICTED_CAST_FROM_ASCII
DEFINES *= QT_NO_CAST_TO_ASCII
DEFINES *= QT_NO_URL_CAST_FROM_STRING

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
