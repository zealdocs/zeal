cmake_minimum_required(VERSION 3.16.3)

include(cmake/SetupVcpkg.cmake)
setup_vcpkg()

# CMake options.
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
set(CMAKE_ERROR_DEPRECATED TRUE)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# VERSION is the next development version, bumped automatically after each release.
# ZEAL_RELEASE_VERSION is the last released version, updated by release-please.
# When both match, the build is treated as a release; otherwise it's a dev build.
project(Zeal
    VERSION 0.8.1 # x-release-please-version
    DESCRIPTION "A simple documentation browser."
    HOMEPAGE_URL "https://zealdocs.org"
    LANGUAGES CXX
)

set(ZEAL_RELEASE_VERSION "0.8.0") # x-release-please-version

option(ZEAL_USE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
if(ZEAL_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_PROGRAM clang-tidy)
    if(CLANG_TIDY_PROGRAM)
        message(STATUS "Using clang-tidy: ${CLANG_TIDY_PROGRAM}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
    else()
        message(FATAL_ERROR "ZEAL_USE_CLANG_TIDY is enabled but clang-tidy was not found.")
    endif()
endif()

option(ZEAL_USE_COMPILER_CACHE "Enable compiler cache (sccache/ccache)" OFF)
if(ZEAL_USE_COMPILER_CACHE)
    find_program(CCACHE_PROGRAM sccache ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Using compiler cache: ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")

        # sccache/ccache don't support MSVC's /Zi (shared PDB). Use /Z7 (embedded debug info) instead.
        # See: https://github.com/mozilla/sccache?tab=readme-ov-file#usage
        if(MSVC)
            foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
                string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
            endforeach()
        endif()
    else()
        message(WARNING "ZEAL_USE_COMPILER_CACHE is enabled but no sccache or ccache found.")
    endif()
endif()

# Project information.
set(PROJECT_COMPANY_NAME "Oleg Shparber")
set(PROJECT_COPYRIGHT "© 2013-2026 Oleg Shparber and other contributors")

# Find available major Qt version. It will be stored in QT_VERSION_MAJOR.
if(NOT ZEAL_USE_QT5)
    find_package(QT NAMES Qt6 COMPONENTS Core)
    set(QT_MINIMUM_VERSION 6.2.0)
endif()

if(NOT QT_FOUND)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core)
    set(QT_MINIMUM_VERSION 5.15.2)
endif()

message(NOTICE "Detected Qt version: ${QT_VERSION}")

# Determine version suffix for non-release builds.
if(PROJECT_VERSION STREQUAL ZEAL_RELEASE_VERSION)
    message(NOTICE "Release build: ${PROJECT_VERSION}")
else()
    include(GitVersionInfo)
    if(Zeal_GIT_AT_TAG)
        message(FATAL_ERROR "Tagged commit but PROJECT_VERSION (${PROJECT_VERSION}) != ZEAL_RELEASE_VERSION (${ZEAL_RELEASE_VERSION}).")
    elseif(Zeal_GIT_VERSION_AHEAD)
        set(ZEAL_VERSION_SUFFIX "-dev.${Zeal_GIT_VERSION_AHEAD}")
    else()
        set(ZEAL_VERSION_SUFFIX "-dev")
    endif()
    message(NOTICE "Dev build: ${PROJECT_VERSION}${ZEAL_VERSION_SUFFIX}")
endif()

set(ZEAL_VERSION_FULL "${Zeal_VERSION}${ZEAL_VERSION_SUFFIX}")
message(NOTICE "Calculated Zeal version: ${ZEAL_VERSION_FULL}")

file(WRITE "${CMAKE_BINARY_DIR}/zeal_version" ${ZEAL_VERSION_FULL})

# A custom target to print the full version.
# Usage: cmake --build --preset release --target zeal_version
add_custom_target(zeal_version
    COMMAND ${CMAKE_COMMAND} -E echo "Zeal version: ${ZEAL_VERSION_FULL}"
    VERBATIM
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

option(BUILD_TESTING "Build the testing suite" OFF)
if(BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(assets)
add_subdirectory(src)
