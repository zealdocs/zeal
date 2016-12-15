ZEAL_LIB_NAME = Core

QT += network

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libarchive
}
win32: {
    LIBS += -larchive_static -lz
}
macx: {
    INCLUDEPATH += /usr/local/opt/libarchive/include
    LIBS += -L/usr/local/opt/libarchive/lib -larchive
    INCLUDEPATH += /usr/local/opt/sqlite/include
    LIBS += -L/usr/local/opt/sqlite/lib -lsqlite3
}
