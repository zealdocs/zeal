add_library(Widgets STATIC
    browserzoomwidget.cpp
    layouthelper.cpp
    searchedit.cpp
    shortcutedit.cpp
    toolbarframe.cpp
)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(Widgets PRIVATE Qt6::Widgets)
