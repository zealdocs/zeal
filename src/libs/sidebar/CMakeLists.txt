add_library(Sidebar STATIC
    container.cpp
    proxyview.cpp
    view.cpp
    viewprovider.cpp
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
target_link_libraries(Sidebar PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
