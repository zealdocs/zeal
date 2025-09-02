add_library(Widgets STATIC
    layouthelper.cpp
    searchedit.cpp
    shortcutedit.cpp
    toolbarframe.cpp
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
target_link_libraries(Widgets PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
