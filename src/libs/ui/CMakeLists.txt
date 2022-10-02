add_subdirectory(qxtglobalshortcut)
add_subdirectory(widgets)

set(Ui_FORMS
    aboutdialog.ui
    docsetsdialog.ui
    mainwindow.ui
    settingsdialog.ui
)

add_library(Ui STATIC
    aboutdialog.cpp
    browsertab.cpp
    docsetlistitemdelegate.cpp
    docsetsdialog.cpp
    mainwindow.cpp
    searchitemdelegate.cpp
    searchsidebar.cpp
    settingsdialog.cpp
    sidebarviewprovider.cpp
    ${Ui_FORMS} # For Qt Creator.
)

target_link_libraries(Ui Browser Sidebar QxtGlobalShortcut Widgets Registry)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineWidgets REQUIRED)
target_link_libraries(Ui Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
