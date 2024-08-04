cmake_minimum_required(VERSION 3.16.3)

# CMake options.
set(CMAKE_ERROR_DEPRECATED TRUE)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Zeal
    VERSION 0.7.2
    DESCRIPTION "A simple documentation browser."
    HOMEPAGE_URL "https://zealdocs.org"
    LANGUAGES CXX
)

# Set to TRUE for a tagged release.
# NOTE: Don't forget to add a new release entry in the AppStream metadata!
if(NOT ZEAL_RELEASE_BUILD AND DEFINED ENV{ZEAL_RELEASE_BUILD})
    set(ZEAL_RELEASE_BUILD $ENV{ZEAL_RELEASE_BUILD})
endif()

# Project information.
set(PROJECT_COMPANY_NAME "Oleg Shparber")
set(PROJECT_COPYRIGHT "© 2013-2024 Oleg Shparber and other contributors")

# Find available major Qt version. It will be stored in QT_VERSION_MAJOR.
if(NOT ZEAL_USE_QT5)
    find_package(QT NAMES Qt6 COMPONENTS Core)
    set(QT_MINIMUM_VERSION 6.2.0)
endif()

if(NOT QT_FOUND)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core)
    set(QT_MINIMUM_VERSION 5.9.5)
endif()

message(NOTICE "Detected Qt version: ${QT_VERSION}")

# Determine version for dev builds.
if(NOT ZEAL_RELEASE_BUILD)
    message(NOTICE "Building unreleased code. Proceed at your own risk!")

    # TODO: Add support for metadata passed from env, e.g. aur, appimage, etc.
    include(GetVersionFromGit)
    if(Zeal_GIT_VERSION_SHA)
        # Extra check in case we forgot to bump version in project() directive.
        if(NOT PROJECT_VERSION_PATCH EQUAL Zeal_GIT_VERSION_PATCH_NEXT)
            message(WARNING "Incorrect patch version! Forgot to bump?")
        endif()

        set(ZEAL_VERSION_SUFFIX "-dev.${Zeal_GIT_VERSION_AHEAD}+${Zeal_GIT_VERSION_SHA}")
    else()
        set(ZEAL_VERSION_SUFFIX "-dev")
    endif()
endif()

set(ZEAL_VERSION_FULL "${Zeal_VERSION}${ZEAL_VERSION_SUFFIX}")
message(NOTICE "Calculated Zeal version: ${ZEAL_VERSION_FULL}")

file(WRITE "${CMAKE_BINARY_DIR}/zeal_version" ${ZEAL_VERSION_FULL})

# A custom target to print the full version.
# Usage: cmake --build build --preset ninja-multi-vcpkg-release --target zeal_version
add_custom_target(zeal_version
    COMMAND ${CMAKE_COMMAND} -E echo "Zeal version: ${ZEAL_VERSION_FULL}"
    VERBATIM
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

add_subdirectory(assets)
add_subdirectory(src)
