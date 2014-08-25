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
ICON = zeal.icns


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
    progressitemdelegate.cpp \
    zealdocsetmetadata.cpp \
    zealdocsetinfo.cpp

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
    progressitemdelegate.h \
    zealdocsetmetadata.h \
    zealdocsetinfo.h

FORMS    += mainwindow.ui \
    zealsettingsdialog.ui


QMAKE_CXXFLAGS += -std=c++11

macx:DEFINES += OSX
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc+
macx:CONFIG += c++11

win32:RC_ICONS = zeal.ico
win32:DEFINES += WIN32 QUAZIP_BUILD
DEFINES += ZEAL_VERSION=\\\"20140215\\\"
LIBS += -lz -L/usr/lib

CONFIG += link_pkgconfig

unix:!macx: LIBS += -lxcb -lxcb-keysyms
unix:!macx: SOURCES += xcb_keysym.cpp
unix:!macx: DEFINES += LINUX

unix:!macx:!no_libappindicator {
    INCLUDEPATH += /usr/include/libappindicator-0.1 \
        /usr/include/gtk-2.0 \
        /usr/lib/gtk-2.0/include
    PKGCONFIG = gtk+-2.0
    LIBS += -lappindicator

    DEFINES += USE_LIBAPPINDICATOR
}

appicons16.path=/usr/share/icons/hicolor/16x16/apps
appicons24.path=/usr/share/icons/hicolor/24x24/apps
appicons32.path=/usr/share/icons/hicolor/32x32/apps
appicons64.path=/usr/share/icons/hicolor/64x64/apps
appicons128.path=/usr/share/icons/hicolor/128x128/apps
appicons16.files=appicons/16/*
appicons24.files=appicons/24/*
appicons32.files=appicons/32/*
appicons64.files=appicons/64/*
appicons128.files=appicons/128/*
icons.path=/usr/share/pixmaps/zeal
icons.files=icons/*
desktop.path=/usr/share/applications
desktop.files=zeal.desktop
unix:INSTALLS += appicons16 appicons24 appicons32 appicons64 appicons128 icons desktop

include (widgets/widgets.pri)
include (quazip/quazip.pri)

RESOURCES += \
    zeal.qrc

OTHER_FILES += \
    zeal.icns
