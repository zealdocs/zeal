include($$SRC_ROOT/common.pri)

TEMPLATE = app

QT += gui widgets sql concurrent

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
}

win32 {
    TARGET = zeal
    RC_ICONS = resources/zeal.ico
}

macx {
    TARGET = Zeal
    ICON = resources/zeal.icns
}
