add_library(Browser STATIC
    searchtoolbar.cpp
    settings.cpp
    urlrequestinterceptor.cpp
    webbridge.cpp
    webcontrol.cpp
    webpage.cpp
    webview.cpp
)

target_link_libraries(Browser)

find_package(Qt5 COMPONENTS WebEngineWidgets WebChannel REQUIRED)
target_link_libraries(Browser Qt5::WebEngineWidgets Qt5::WebChannel)
