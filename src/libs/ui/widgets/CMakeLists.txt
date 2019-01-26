add_library(Widgets
    searchedit.cpp
    searchtoolbar.cpp
    shortcutedit.cpp
    toolbarframe.cpp
    webview.cpp
    webviewtab.cpp
)

find_package(Qt5 COMPONENTS WebKitWidgets REQUIRED)
target_link_libraries(Widgets Qt5::WebKitWidgets)
