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
VERSION = 0.5.0
DEFINES += ZEAL_VERSION=\\\"$${VERSION}\\\"

# Portable build
CONFIG(zeal_portable) {
    message("Portable build: Yes.")
    DEFINES += PORTABLE_BUILD
}

# Unix installation prefix
unix:!macx {
    isEmpty(PREFIX): PREFIX = /usr
}
