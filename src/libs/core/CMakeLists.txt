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

include(FindPackageHandleStandardArgs)

# Configure toml++ (header-only).
# 3.4.0 renamed toml.h to toml.hpp.
# QUIET: falling back to the bundled copy is not a failure.
find_package(tomlplusplus 3.4.0 CONFIG QUIET)
if(tomlplusplus_FOUND)
    find_package_handle_standard_args(tomlplusplus CONFIG_MODE)
    target_link_libraries(Core PRIVATE tomlplusplus::tomlplusplus)
else()
    set(tomlplusplus_bundled_dir "${CMAKE_SOURCE_DIR}/src/contrib/tomlplusplus")
    zeal_bundled_version(tomlplusplus_bundled_version
        "${tomlplusplus_bundled_dir}/toml++/toml.hpp"
        "^// toml\\+\\+ v")
    find_package_handle_standard_args(tomlplusplus
        REQUIRED_VARS tomlplusplus_bundled_dir
        VERSION_VAR tomlplusplus_bundled_version)
    target_include_directories(Core SYSTEM PRIVATE "${tomlplusplus_bundled_dir}")
endif()

# Configure cpp-httplib.
add_definitions(-DCPPHTTPLIB_USE_POLL)

# SameMinorVersion in httplib's config pins 0.x, so the floor is checked here instead.
# 0.11.0 added the set_content(const char *, size_t, ...) overload.
find_package(httplib CONFIG QUIET)
if(httplib_FOUND AND httplib_VERSION VERSION_LESS 0.11.0)
    message(STATUS "Ignoring cpp-httplib ${httplib_VERSION}, 0.11.0 or newer required")
    set(httplib_FOUND FALSE)
endif()

if(httplib_FOUND)
    find_package_handle_standard_args(httplib CONFIG_MODE)
    target_link_libraries(Core PRIVATE httplib::httplib)
else()
    set(httplib_bundled_dir "${CMAKE_SOURCE_DIR}/src/contrib/cpp-httplib")
    zeal_bundled_version(httplib_bundled_version
        "${httplib_bundled_dir}/httplib.h"
        "^#define CPPHTTPLIB_VERSION ")
    find_package_handle_standard_args(httplib
        REQUIRED_VARS httplib_bundled_dir
        VERSION_VAR httplib_bundled_version)
    target_include_directories(Core SYSTEM PRIVATE "${httplib_bundled_dir}")
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
