add_library(Sidebar STATIC
    container.cpp
    proxyview.cpp
    view.cpp
    viewprovider.cpp
)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(Sidebar PRIVATE Qt6::Widgets)
