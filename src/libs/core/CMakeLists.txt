add_library(Core STATIC
    application.cpp
    applicationsingleton.cpp
    extractor.cpp
    filemanager.cpp
    httpserver.cpp
    networkaccessmanager.cpp
    settings.cpp

    # Show headers without .cpp in Qt Creator.
    httplib.h
)

# Configure cpp-httplib.
add_definitions(-DCPPHTTPLIB_USE_POLL)

target_link_libraries(Core Registry Ui)

find_package(Qt5 COMPONENTS Network WebEngine Widgets REQUIRED)
target_link_libraries(Core Qt5::Network Qt5::WebEngine Qt5::Widgets)

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
target_link_libraries(Core ${LibArchive_LIBRARIES})

# Required by cpp-httplib.
if(NOT WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(Core Threads::Threads)
endif()
