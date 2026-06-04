add_library(Widgets STATIC
    browserzoomwidget.cpp
    emptystatelabel.cpp
    iconhelper.cpp
    layouthelper.cpp
    proxystyle.cpp
    searchedit.cpp
    shortcutedit.cpp
    tabbar.cpp
    toolbarframe.cpp
)

find_package(Qt6 COMPONENTS Svg Widgets REQUIRED)
target_link_libraries(Widgets PRIVATE Qt6::Svg Qt6::Widgets)

zeal_attach_qt_pch(Widgets <QWidget>)
