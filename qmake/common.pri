# Shared build options
#
# This file must be included at the top of every non-subdirs .pro file.
# Use:
# include($$ZEAL_COMMON_PRI)

# Compilation settings
CONFIG += c++11 silent

# Shared include path
INCLUDEPATH += $$SRC_ROOT/src/libs
LIBS = -L$$BUILD_ROOT/.lib

# QString options
DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES *= QT_RESTRICTED_CAST_FROM_ASCII
DEFINES *= QT_NO_CAST_TO_ASCII
DEFINES *= QT_NO_URL_CAST_FROM_STRING

# Workaround for AppVeyor: Do not warn if the library had to be created.
# Based on https://codereview.qt-project.org/150326
win32-g++:lessThan(QT_VERSION, "5.6.0") {
    QMAKE_LIB = ar -rc
}

# Keep build directory clean
MOC_DIR = $$BUILD_ROOT/.moc
OBJECTS_DIR = $$BUILD_ROOT/.obj
RCC_DIR = $$BUILD_ROOT/.rcc
UI_DIR = $$BUILD_ROOT/.ui

# Application version
VERSION = 0.3.1
DEFINES += ZEAL_VERSION=\\\"$${VERSION}\\\"

# Browser engine
CONFIG(zeal_qtwebengine) {
    qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
    else: error("Qt WebEngine is not available.")
} else {
    qtHaveModule(webkitwidgets): BROWSER_ENGINE = qtwebkit
    else: qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
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
}

# Unix installation prefix
unix:!macx {
    isEmpty(PREFIX): PREFIX = /usr
}

unix:!macx:packagesExist(appindicator-0.1) {
    CONFIG += link_pkgconfig
    PKGCONFIG += appindicator-0.1 gtk+-2.0
    DEFINES += USE_APPINDICATOR
    message("AppIndicator support: Yes.")
}
