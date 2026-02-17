add_library(Util STATIC
    fuzzy.cpp
    humanizer.cpp
    plist.cpp
    sqlitedatabase.cpp

    # Show headers without .cpp in Qt Creator.
    caseinsensitivemap.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
target_link_libraries(Util PRIVATE Qt${QT_VERSION_MAJOR}::Core)

find_package(SQLite3 REQUIRED)
target_link_libraries(Util PRIVATE SQLite::SQLite3)

# TODO: Do not export SQLite headers.
target_include_directories(Util PUBLIC ${SQLite3_INCLUDE_DIRS})

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
