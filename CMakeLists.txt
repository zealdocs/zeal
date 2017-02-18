cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Zeal VERSION 0.3.1)

# Project information.
if(APPLE)
    set(PROJECT_OUTPUT_NAME "Zeal")
else()
    set(PROJECT_OUTPUT_NAME "zeal")
endif()

set(PROJECT_COMPANY_NAME "ZealDocs")

# CMake <= 3.8.0 doesn't support Unicode on Windows:
# https://gitlab.kitware.com/cmake/cmake/commit/335a29cb07a8d014a9ed4d4dbf25e49da3a78b33
# Comparing with 3.8.1 to catch 3.8.0-rc1: https://gitlab.kitware.com/cmake/cmake/issues/16656
if(WIN32 AND CMAKE_VERSION VERSION_LESS 3.8.1)
    string(TIMESTAMP PROJECT_COPYRIGHT "(C) 2015-%Y Oleg Shparber" UTC)
else()
    string(TIMESTAMP PROJECT_COPYRIGHT "© 2015-%Y Oleg Shparber" UTC)
endif()

set(PROJECT_DESCRIPTION "A simple documentation browser.")
set(PROJECT_URL "https://zealdocs.org")

add_subdirectory(assets)
add_subdirectory(src)
