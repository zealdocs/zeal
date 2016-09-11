ZEAL_LIB_NAME = Ui

QT += widgets

unix:!macx {
    packagesExist(appindicator-0.1) {
        CONFIG += link_pkgconfig
        PKGCONFIG += appindicator-0.1 gtk+-2.0
        DEFINES += USE_APPINDICATOR
        message("AppIndicator support: Yes.")
    } else {
        message("AppIndicator support: No.")
    }
}

# QxtGlobalShortcut dependencies
unix:!macx {
    QT += x11extras

    CONFIG += link_pkgconfig
    PKGCONFIG += x11 xcb xcb-keysyms
}

macx {
    LIBS += -framework Carbon
}
