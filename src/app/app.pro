include($$ZEAL_COMMON_PRI)

TEMPLATE = app

QT += gui widgets sql concurrent

SOURCES += \
    main.cpp

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

# FIXME: Hardcoded link line & cyclic dependencies.
LIBS += -lCore -lUi -lRegistry -lUtil

# Depend on all dependencies of libraries
for(lib_dir, $$list($$files($$SRC_ROOT/src/libs/*))) {
    !equals(lib_dir, $$SRC_ROOT/src/libs/libs.pro) {
        include($$lib_dir/$$basename(lib_dir).pri)
#        LIBS += -l$$ZEAL_LIB_NAME
    }
}
