add_subdirectory(qxtglobalshortcut)
add_subdirectory(widgets)

set(Ui_FORMS
    aboutdialog.ui
    docsetsdialog.ui
    mainwindow.ui
    settingsdialog.ui
)

add_library(Ui
    aboutdialog.cpp
    docsetlistitemdelegate.cpp
    docsetsdialog.cpp
    mainwindow.cpp
    progressitemdelegate.cpp
    searchitemdelegate.cpp
    settingsdialog.cpp
    ${Ui_FORMS} # For Qt Creator.
)

target_link_libraries(Ui Browser QxtGlobalShortcut Widgets Registry)

find_package(Qt5 COMPONENTS WebKitWidgets REQUIRED)
target_link_libraries(Ui Qt5::WebKitWidgets)
