add_subdirectory(qxtglobalshortcut)
add_subdirectory(widgets)

set(Ui_FORMS
    aboutdialog.ui
    docsetsdialog.ui
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
    windowmanager.cpp
    ${Ui_FORMS} # For Qt Creator.
)

target_link_libraries(Ui PRIVATE Browser Sidebar QxtGlobalShortcut Widgets Registry Util)

find_package(Qt6 COMPONENTS WebEngineWidgets REQUIRED)
target_link_libraries(Ui PRIVATE Qt6::WebEngineWidgets)
