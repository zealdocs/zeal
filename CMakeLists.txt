cmake_minimum_required(VERSION 3.10.2)

# CMake options.
set(CMAKE_ERROR_DEPRECATED TRUE)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Zeal VERSION 0.6.2)

# Set to TRUE for a tagged release.
# NOTE: Don't forget to add a new release entry in the AppStream metadata!
set(RELEASE_VERSION FALSE)

# Project information.
if(APPLE)
    set(PROJECT_OUTPUT_NAME "Zeal")
else()
    set(PROJECT_OUTPUT_NAME "zeal")
endif()

set(PROJECT_COMPANY_NAME "ZealDocs")
set(PROJECT_COPYRIGHT "© 2013-2020 Oleg Shparber and other contributors")
set(PROJECT_DESCRIPTION "A simple documentation browser.")
set(PROJECT_URL "https://zealdocs.org")

set(QT_MINIMUM_VERSION 5.9.5)

# Determine version for dev builds.
if(NOT RELEASE_VERSION)
    message(NOTICE "Building unreleased code. Proceed at your own risk!")

    # TODO: Add support for metadata passed from env, e.g. aur, appimage, etc.
    include(GetVersionFromGit)
    if(Zeal_GIT_VERSION_SHA)
        # Extra check in case we forgot to bump version in project() directive.
        if(NOT PROJECT_VERSION_PATCH EQUAL Zeal_GIT_VERSION_PATCH_NEXT)
            message(FATAL_ERROR "Incorrect patch version! Forgot to bump?")
        endif()

        set(ZEAL_VERSION_SUFFIX "-dev.${Zeal_GIT_VERSION_AHEAD}+${Zeal_GIT_VERSION_SHA}")
    else()
        set(ZEAL_VERSION_SUFFIX "-dev")
    endif()
endif()

set(ZEAL_VERSION_FULL "${Zeal_VERSION}${ZEAL_VERSION_SUFFIX}")

# For development builds insert an extra release in the AppStream metadata.
if(NOT RELEASE_VERSION)
    string(TIMESTAMP ZEAL_APPSTREAM_DEV_RELEASE "\n    <release date=\"%Y-%m-%d\" version=\"${ZEAL_VERSION_FULL}\" type=\"development\" />")
endif()

# A custom target to print the full version.
# Usage: cmake --build build --target zeal_version
add_custom_target(zeal_version
    COMMAND ${CMAKE_COMMAND} -E echo "Zeal version: ${ZEAL_VERSION_FULL}"
    VERBATIM
)

add_subdirectory(assets)
add_subdirectory(src)
