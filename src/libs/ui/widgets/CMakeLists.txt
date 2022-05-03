add_library(Widgets STATIC
    layouthelper.cpp
    searchedit.cpp
    shortcutedit.cpp
    toolbarframe.cpp
)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(Widgets Qt6::Widgets)
