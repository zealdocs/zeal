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

find_package(SQLite3 REQUIRED)
target_link_libraries(Util SQLite::SQLite3)

# TODO: Do not export SQLite headers.
target_include_directories(Util PUBLIC ${SQLite3_INCLUDE_DIRS})
