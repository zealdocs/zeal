add_library(Browser STATIC
    searchtoolbar.cpp
    settings.cpp
    urlrequestinterceptor.cpp
    webbridge.cpp
    webcontrol.cpp
    webpage.cpp
    webview.cpp
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebChannel WebEngineWidgets REQUIRED)
target_link_libraries(Browser PRIVATE Qt${QT_VERSION_MAJOR}::WebChannel Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
