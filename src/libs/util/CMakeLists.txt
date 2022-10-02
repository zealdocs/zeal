# Nothing to moc here, so avoid empty build units.
set(CMAKE_AUTOMOC OFF)

add_library(Util STATIC
    plist.cpp
    sqlitedatabase.cpp

    # Show headers without .cpp in Qt Creator.
    caseinsensitivemap.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
target_link_libraries(Util Qt${QT_VERSION_MAJOR}::Core)

find_package(SQLite REQUIRED)
include_directories(${SQLite_INCLUDE_DIR})
target_link_libraries(Util ${SQLite_LIBRARY})

# TODO: Do not export SQLite headers.
target_include_directories(Util PUBLIC ${SQLite_INCLUDE_DIR})
