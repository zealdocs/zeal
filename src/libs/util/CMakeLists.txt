add_library(Util STATIC
    database.cpp
    fuzzy.cpp
    humanizer.cpp
    plist.cpp
    statement.cpp
    tarixarchive.cpp

    # Show headers without .cpp in Qt Creator.
    caseinsensitivemap.h
)

find_package(Qt6 COMPONENTS Core REQUIRED)
target_link_libraries(Util PRIVATE Qt6::Core)

zeal_attach_qt_pch(Util)

find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()
target_link_libraries(Util PRIVATE SQLite3::SQLite3)

# TODO: Do not export SQLite headers.
target_include_directories(Util PUBLIC ${SQLite3_INCLUDE_DIRS})

find_package(LibArchive REQUIRED)
target_link_libraries(Util PRIVATE LibArchive::LibArchive)

find_package(ZLIB REQUIRED)
target_link_libraries(Util PRIVATE ZLIB::ZLIB)

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
