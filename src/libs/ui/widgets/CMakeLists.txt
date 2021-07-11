add_library(Widgets STATIC
    layouthelper.cpp
    searchedit.cpp
    shortcutedit.cpp
    toolbarframe.cpp
    browserzoomwidget.cpp
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(Widgets Qt5::Widgets)
