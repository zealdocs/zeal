add_library(Widgets STATIC
    searchedit.cpp
    shortcutedit.cpp
    toolbarframe.cpp
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(Widgets Qt5::Widgets)
