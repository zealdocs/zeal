cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Zeal VERSION 0.6.0)

# Project information.
if(APPLE)
    set(PROJECT_OUTPUT_NAME "Zeal")
else()
    set(PROJECT_OUTPUT_NAME "zeal")
endif()

set(PROJECT_COMPANY_NAME "ZealDocs")
string(TIMESTAMP PROJECT_COPYRIGHT "© 2015-%Y Oleg Shparber" UTC)
set(PROJECT_DESCRIPTION "A simple documentation browser.")
set(PROJECT_URL "https://zealdocs.org")

set(QT_MINIMUM_VERSION 5.5.1)

add_subdirectory(assets)
add_subdirectory(src)
