include($$ZEAL_LIBRARY_PRI)

HEADERS += \
    $$files(*.h) \
    $$files(widgets/*.h)

SOURCES += \
    $$files(*.cpp) \
    $$files(widgets/*.cpp)

FORMS += \
    $$files(*.ui)

include(qxtglobalshortcut/qxtglobalshortcut.pri)
