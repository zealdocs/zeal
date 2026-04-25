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

if(TARGET LibArchive::LibArchive)
    target_link_libraries(Core PRIVATE LibArchive::LibArchive)
else()
    include_directories(${LibArchive_INCLUDE_DIRS})
    target_link_libraries(Core PRIVATE ${LibArchive_LIBRARIES})
endif()

# Configure toml++ (header-only).
find_package(tomlplusplus CONFIG QUIET)
if(tomlplusplus_FOUND)
    target_link_libraries(Core PRIVATE tomlplusplus::tomlplusplus)
else()
    # Use bundled version of toml++ if not found.
    target_include_directories(Core PRIVATE "${CMAKE_SOURCE_DIR}/src/contrib/tomlplusplus")
endif()

# Configure cpp-httplib.
add_definitions(-DCPPHTTPLIB_USE_POLL)

find_package(httplib CONFIG QUIET)
if(httplib_FOUND)
    target_link_libraries(Core PRIVATE httplib::httplib)
else()
    # Use bundled version of cpp-httplib if not found.
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
