set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find includes in corresponding build directories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

## Build options
option(ZEAL_PORTABLE_BUILD "Build portable version")
if(ZEAL_PORTABLE_BUILD)
    add_definitions(-DPORTABLE_BUILD)
endif()

## Macros.
add_definitions(-DZEAL_VERSION="${ZEAL_VERSION_FULL}")

# QString options
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_RESTRICTED_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

## Handle moc, uic, and rcc files.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(libs)
add_subdirectory(libs)

add_subdirectory(app)
