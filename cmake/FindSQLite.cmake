# FindSQLite
# ----------
#
# Finds SQLite headers and libraries.
#
# SQLite_FOUND          - True if SQLite is found.
# SQLite_INCLUDE_DIR    - Path to SQLite header files.
# SQLite_LIBRARY        - SQLite library to link to.
# SQLite_VERSION_STRING - The version of SQLite found.

# A simple fallback to pkg-config.
if(NOT WIN32)
    find_package(PkgConfig QUIET)
    pkg_search_module(PC_SQLITE QUIET sqlite3)
endif()

find_path(SQLite_INCLUDE_DIR NAMES sqlite3.h
    PATHS
    ${PC_SQLITE_INCLUDEDIR}
    ${PC_SQLITE_INCLUDE_DIRS}
)

find_library(SQLite_LIBRARY NAMES sqlite3
    PATHS
    ${PC_SQLITE_LIBDIR}
    ${PC_SQLITE_LIBRARY_DIRS}
)

if(PC_SQLITE_VERSION)
    set(SQLite_VERSION_STRING ${PC_SQLITE_VERSION})
elseif(SQLite_INCLUDE_DIR AND EXISTS "${SQLite_INCLUDE_DIR}/sqlite3.h")
    # Parse sqlite3.h for the version macro:
    # #define SQLITE_VERSION        "3.20.1"
    set(_SQLite_VERSION_REGEX "^#define[ \t]+SQLITE_VERSION[ \t]+\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\".*$")
    file(STRINGS "${SQLite_INCLUDE_DIR}/sqlite3.h" _SQLite_VERSION_MACRO LIMIT_COUNT 1 REGEX "${_SQLite_VERSION_REGEX}")
    if(_SQLite_VERSION_MACRO)
        string(REGEX REPLACE "${_SQLite_VERSION_REGEX}" "\\1.\\2.\\3" SQLite_VERSION_STRING "${_SQLite_VERSION_MACRO}")
    endif()
    unset(_SQLite_VERSION_MACRO)
    unset(_SQLite_VERSION_REGEX)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLite
    REQUIRED_VARS SQLite_LIBRARY SQLite_INCLUDE_DIR
    VERSION_VAR SQLite_VERSION_STRING
)

mark_as_advanced(SQLite_INCLUDE_DIR SQLite_LIBRARY)
