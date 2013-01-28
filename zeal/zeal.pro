#-------------------------------------------------
#
# Project created by QtCreator 2013-01-18T22:28:41
#
#-------------------------------------------------

QT       += core gui widgets webkitwidgets sql


TARGET = zeal
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    zeallistmodel.cpp \
    zealsearchmodel.cpp \
    zealdocsetsregistry.cpp \
    zealsearchresult.cpp \
    zealsearchedit.cpp \
    zealnativeeventfilter.cpp \
    lineedit.cpp \
    searchablewebview.cpp

HEADERS  += mainwindow.h \
    zeallistmodel.h \
    zealsearchmodel.h \
    zealdocsetsregistry.h \
    zealsearchresult.h \
    zealsearchedit.h \
    zealnativeeventfilter.h \
    lineedit.h \
    searchablewebview.h

FORMS    += mainwindow.ui


QMAKE_CXXFLAGS += -std=c++11

win32:DEFINES += WIN32

unix:!macx: LIBS += -lxcb-keysyms
