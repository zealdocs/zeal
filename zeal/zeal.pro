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
    zealsearchquery.cpp

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
    zealsearchquery.h

FORMS    += mainwindow.ui \
    zealsettingsdialog.ui


QMAKE_CXXFLAGS += -std=c++11

win32:DEFINES += WIN32 QUAZIP_BUILD
LIBS += -lz

unix:!macx: LIBS += -lxcb -lxcb-keysyms
unix:!macx: SOURCES += xcb_keysym.cpp

icons.path=/usr/share/pixmaps/zeal
icons.files=icons/*
unix:INSTALLS += icons

include (widgets/widgets.pri)
include (quazip/quazip.pri)
