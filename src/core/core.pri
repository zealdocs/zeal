HEADERS += $$files($$PWD/*.h)
SOURCES += $$files($$PWD/*.cpp)

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libarchive
}
win32: {
    LIBS += -larchive_static -lz
}

macx: {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -larchive
}
