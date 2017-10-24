add_library(Core
    application.cpp
    applicationsingleton.cpp
    extractor.cpp
    filemanager.cpp
    networkaccessmanager.cpp
    settings.cpp
)

target_link_libraries(Core Registry Ui)

find_package(Qt5 COMPONENTS Network WebKit Widgets REQUIRED)
target_link_libraries(Core Qt5::Network Qt5::WebKit Qt5::Widgets)

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
target_link_libraries(Core ${LibArchive_LIBRARIES})
