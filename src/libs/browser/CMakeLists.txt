add_library(Browser STATIC
    searchtoolbar.cpp
    settings.cpp
    urlrequestinterceptor.cpp
    webbridge.cpp
    webcontrol.cpp
    webpage.cpp
    webview.cpp
)

find_package(Qt6 COMPONENTS WebChannel WebEngineWidgets REQUIRED)
target_link_libraries(Browser PRIVATE Qt6::WebChannel Qt6::WebEngineWidgets)
