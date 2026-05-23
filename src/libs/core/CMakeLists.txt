add_library(Core STATIC
    application.cpp
    applicationsingleton.cpp
    extractor.cpp
    filemanager.cpp
    httpserver.cpp
    networkaccessmanager.cpp
    settings.cpp
    session.cpp
)

target_link_libraries(Core PRIVATE Registry)

find_package(Qt6 COMPONENTS Network WebEngineCore Widgets REQUIRED)
target_link_libraries(Core PRIVATE
    Qt6::Network
    Qt6::WebEngineCore
    Qt6::Widgets
)

zeal_attach_qt_pch(Core <QDir>)

find_package(LibArchive REQUIRED)
target_link_libraries(Core PRIVATE LibArchive::LibArchive)

# Configure toml++ (header-only).
find_package(tomlplusplus CONFIG)
if(tomlplusplus_FOUND)
    target_link_libraries(Core PRIVATE tomlplusplus::tomlplusplus)
else()
    message(STATUS "Using bundled tomlplusplus")
    target_include_directories(Core PRIVATE "${CMAKE_SOURCE_DIR}/src/contrib/tomlplusplus")
endif()

# Configure cpp-httplib.
add_definitions(-DCPPHTTPLIB_USE_POLL)

find_package(httplib CONFIG)
if(httplib_FOUND)
    target_link_libraries(Core PRIVATE httplib::httplib)
else()
    message(STATUS "Using bundled cpp-httplib")
    include_directories("${CMAKE_SOURCE_DIR}/src/contrib/cpp-httplib")
endif()

# Required by cpp-httplib.
if(NOT WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(Core PRIVATE Threads::Threads)
endif()

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
