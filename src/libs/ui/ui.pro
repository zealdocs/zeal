include($$ZEAL_LIBRARY_PRI)

HEADERS += \
    $$files(*.h) \
    $$files(widgets/*.h)

SOURCES += \
    $$files(*.cpp) \
    $$files(widgets/*.cpp)

FORMS += \
    $$files(forms/*.ui)

include(qxtglobalshortcut/qxtglobalshortcut.pri)
