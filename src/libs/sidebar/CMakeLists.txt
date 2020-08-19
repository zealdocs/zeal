add_library(Sidebar STATIC
    container.cpp
    proxyview.cpp
    view.cpp
    viewprovider.cpp
)

target_link_libraries(Sidebar)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(Sidebar Qt5::Widgets)
