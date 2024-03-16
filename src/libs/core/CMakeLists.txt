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

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network WebEngineCore Widgets REQUIRED)
target_link_libraries(Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::WebEngineCore Qt${QT_VERSION_MAJOR}::Widgets)

if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS WebEngineCore Widgets REQUIRED)
    target_link_libraries(Core Qt6::WebEngineCore)
else()
    find_package(Qt5 COMPONENTS WebEngine Widgets REQUIRED)
    target_link_libraries(Core Qt5::WebEngine)
endif()

find_package(LibArchive QUIET)
if(NOT LibArchive_FOUND)
    find_path(LibArchive_INCLUDE_DIRS archive.h
        PATHS /opt/homebrew/opt/libarchive/include /usr/local/opt/libarchive/include
        REQUIRED
    )
    find_library(LibArchive_LIBRARIES
        NAMES archive libarchive
        PATHS /opt/homebrew/opt/libarchive/lib /usr/local/opt/libarchive/lib
        REQUIRED
        NO_DEFAULT_PATH
    )
endif()

if((CMAKE_VERSION VERSION_GREATER_EQUAL 3.17.0) AND (TARGET LibArchive::LibArchive))
    target_link_libraries(Core LibArchive::LibArchive)
else()
    include_directories(${LibArchive_INCLUDE_DIRS})
    target_link_libraries(Core ${LibArchive_LIBRARIES})
endif()

# Required by cpp-httplib.
if(NOT WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(Core Threads::Threads)
endif()
