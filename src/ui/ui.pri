HEADERS += \
    $$files($$PWD/*.h) \
    $$files($$PWD/widgets/*.h)

SOURCES += \
    $$files($$PWD/*.cpp) \
    $$files($$PWD/widgets/*.cpp)

FORMS += \
    $$files($$PWD/forms/*.ui)

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += x11

    QMAKE_DEL_DIR = rmdir --ignore-fail-on-non-empty

    packagesExist(appindicator) {
        PKGCONFIG += appindicator
        DEFINES += USE_LIBAPPINDICATOR
    }
}
