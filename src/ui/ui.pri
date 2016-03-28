HEADERS += \
    $$files($$PWD/*.h) \
    $$files($$PWD/widgets/*.h)

SOURCES += \
    $$files($$PWD/*.cpp) \
    $$files($$PWD/widgets/*.cpp)

FORMS += \
    $$files($$PWD/forms/*.ui)

unix:!macx {
    packagesExist(appindicator-0.1) {
        CONFIG += link_pkgconfig
        PKGCONFIG += appindicator-0.1 gtk+-2.0
        DEFINES += USE_APPINDICATOR
        message("AppIndicator support: Yes")
    } else {
        message("AppIndicator support: No")
    }
}
