#-------------------------------------------------
#
# Project created by QtCreator 2013-01-18T22:28:41
#
#-------------------------------------------------

QT       += core gui widgets webkitwidgets sql gui-private xml


TARGET = zeal
target.path = /usr/bin
INSTALLS = target
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    zeallistmodel.cpp \
    zealsearchmodel.cpp \
    zealdocsetsregistry.cpp \
    zealsearchresult.cpp \
    zealnativeeventfilter.cpp \
    lineedit.cpp \
    zealsearchitemdelegate.cpp \
    zealsearchitemstyle.cpp \
    zealsettingsdialog.cpp \
    zealnetworkaccessmanager.cpp \
    zealsearchquery.cpp \
    progressitemdelegate.cpp

HEADERS  += mainwindow.h \
    zeallistmodel.h \
    zealsearchmodel.h \
    zealdocsetsregistry.h \
    zealsearchresult.h \
    zealnativeeventfilter.h \
    lineedit.h \
    zealsearchitemdelegate.h \
    zealsearchitemstyle.h \
    zealsettingsdialog.h \
    xcb_keysym.h \
    zealnetworkaccessmanager.h \
    zealsearchquery.h \
    progressitemdelegate.h

FORMS    += mainwindow.ui \
    zealsettingsdialog.ui


QMAKE_CXXFLAGS += -std=c++11

macx:DEFINES += OSX
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc+
macx:CONFIG += c++11

win32:DEFINES += WIN32 QUAZIP_BUILD
DEFINES += ZEAL_VERSION=\\\"20140122\\\"
LIBS += -lz

unix:!macx: LIBS += -lxcb -lxcb-keysyms
unix:!macx: SOURCES += xcb_keysym.cpp
unix:!macx: DEFINES += LINUX

icons.path=/usr/share/pixmaps/zeal
icons.files=icons/*
desktop.path=/usr/share/applications
desktop.files=zeal.desktop
unix:INSTALLS += icons desktop

include (widgets/widgets.pri)
include (quazip/quazip.pri)
