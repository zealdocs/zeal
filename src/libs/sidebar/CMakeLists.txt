add_library(Sidebar STATIC
    container.cpp
    proxyview.cpp
    view.cpp
    viewprovider.cpp
)

target_link_libraries(Sidebar)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
target_link_libraries(Sidebar Qt${QT_VERSION_MAJOR}::Widgets)
