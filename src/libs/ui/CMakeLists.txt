add_subdirectory(qxtglobalshortcut)

list(APPEND Widgets_SOURCES
    widgets/searchedit.cpp
    widgets/searchtoolbar.cpp
    widgets/shortcutedit.cpp
    widgets/toolbarframe.cpp
    widgets/webview.cpp
    widgets/webviewtab.cpp
)

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
    webbridge.cpp
    ${Widgets_SOURCES}
    ${Ui_FORMS} # For Qt Creator.
)

target_link_libraries(Ui QxtGlobalShortcut Registry)

find_package(Qt5 COMPONENTS WebKitWidgets REQUIRED)
target_link_libraries(Ui Qt5::WebKitWidgets)
