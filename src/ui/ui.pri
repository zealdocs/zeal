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

    packagesExist(appindicator-0.1) {
        message("AppIndicator support enabled")
        PKGCONFIG += appindicator-0.1 gtk+-2.0
        DEFINES += USE_APPINDICATOR
    }
}
