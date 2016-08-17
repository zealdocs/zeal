# Shared build options
#
# This file must be included at the top of every non-subdirs .pro file.
# Use:
# include($$SRC_ROOT/common.pri)

# Compilation settings
CONFIG += c++11 silent

# QString options
DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES *= QT_RESTRICTED_CAST_FROM_ASCII
DEFINES *= QT_NO_CAST_TO_ASCII
DEFINES *= QT_NO_URL_CAST_FROM_STRING

# Keep build directory clean
MOC_DIR = $$BUILD_ROOT/.moc
OBJECTS_DIR = $$BUILD_ROOT/.obj
RCC_DIR = $$BUILD_ROOT/.rcc
UI_DIR = $$BUILD_ROOT/.ui

# Application version
VERSION = 0.2.1
DEFINES += ZEAL_VERSION=\\\"$${VERSION}\\\"

# Browser engine
CONFIG(zeal_qtwebkit) {
    qtHaveModule(webkitwidgets): BROWSER_ENGINE = qtwebkit
    else: error("Qt WebKit is not available.")
} else:CONFIG(zeal_qtwebengine) {
    qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
    else: error("Qt WebEngine is not available.")
} else {
    qtHaveModule(webenginewidgets): BROWSER_ENGINE = qtwebengine
    else: qtHaveModule(webkitwidgets): BROWSER_ENGINE = qtwebkit
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
} else {
    message("Portable build: No.")
}

# Unix installation prefix
unix:!macx {
    isEmpty(PREFIX): PREFIX = /usr
    message("Install prefix: $$PREFIX")
    target.path = $$PREFIX/bin

    # Always install target
    INSTALLS += target
}
