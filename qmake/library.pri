include($$ZEAL_COMMON_PRI)
include($$replace(_PRO_FILE_PWD_, ([^/]+$), \\1/\\1.pri))

TEMPLATE = lib
CONFIG += staticlib

DESTDIR = $$BUILD_ROOT/.lib
TARGET = $$ZEAL_LIB_NAME
