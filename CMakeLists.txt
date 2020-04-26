cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Zeal VERSION 0.6.1)
set(RELEASE_DATE 2018-09-28)

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

add_subdirectory(assets)
add_subdirectory(src)
