ZEAL_LIB_NAME = Core

QT += network

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libarchive
}
win32: {
    LIBS += -larchive_static -lz
}
