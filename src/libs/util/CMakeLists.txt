# Nothing to moc here, so avoid empty build units.
set(CMAKE_AUTOMOC OFF)

add_library(Util
    plist.cpp
    sqlitedatabase.cpp
    version.cpp
)

find_package(Qt5Core REQUIRED)
target_link_libraries(Util Qt5::Core)

find_package(SQLite REQUIRED)
include_directories(${SQLite_INCLUDE_DIR})
target_link_libraries(Util ${SQLite_LIBRARY})

# TODO: Do not export SQLite headers.
target_include_directories(Util PUBLIC ${SQLite_INCLUDE_DIR})
