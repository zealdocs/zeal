if(CPACK_SOURCE_INSTALLED_DIRECTORIES)
    message(DEBUG "Skipping package signing for source package generator.")
    return()
endif()

# A package without the bundled runtime crashes on startup (issue #1658).
if(NOT CPACK_ZEAL_RUNTIME_LIBS)
    message(FATAL_ERROR "No MSVC runtime libraries were resolved at configure time.")
endif()

foreach(_lib ${CPACK_ZEAL_RUNTIME_LIBS})
    if(NOT EXISTS "${CPACK_TEMPORARY_DIRECTORY}/${_lib}")
        message(FATAL_ERROR "MSVC runtime ${_lib} is missing from the package.")
    endif()
endforeach()

# Every library the app or its plugins load, directly or through other libraries, must
# be in the package unless it comes with Windows. Excluding the Windows directory also
# keeps the search from descending into system libraries.
# TODO: Once CMake 4.3 is the minimum, drop the if(POLICY) check and backslash matching.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

file(GLOB_RECURSE _staged_libs "${CPACK_TEMPORARY_DIRECTORY}/*.dll")

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${CPACK_TEMPORARY_DIRECTORY}/${CPACK_ZEAL_EXECUTABLE}"
    MODULES ${_staged_libs}
    DIRECTORIES "${CPACK_TEMPORARY_DIRECTORY}"
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES "^[A-Za-z]:[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\]"
    RESOLVED_DEPENDENCIES_VAR _resolved_libs
    UNRESOLVED_DEPENDENCIES_VAR _missing_libs
)

# Anything found outside the package, e.g. in a Qt installation on PATH, is missing too.
foreach(_lib ${_resolved_libs})
    cmake_path(GET _lib PARENT_PATH _lib_dir)
    if(NOT _lib_dir STREQUAL CPACK_TEMPORARY_DIRECTORY)
        list(APPEND _missing_libs "${_lib}")
    endif()
endforeach()

if(_missing_libs)
    list(JOIN _missing_libs ", " _missing_libs)
    message(FATAL_ERROR "Libraries missing from the package: ${_missing_libs}")
endif()

# Sign the app and the vcpkg libraries deployed next to it. Qt and the MSVC runtime
# are shipped as their vendors built them.
set(_file_list "${CPACK_ZEAL_EXECUTABLE}")

if(CPACK_ZEAL_VCPKG_BIN_DIR)
    file(GLOB _vcpkg_libs RELATIVE "${CPACK_ZEAL_VCPKG_BIN_DIR}" "${CPACK_ZEAL_VCPKG_BIN_DIR}/*.dll")
    foreach(_lib ${_vcpkg_libs})
        if(EXISTS "${CPACK_TEMPORARY_DIRECTORY}/${_lib}")
            list(APPEND _file_list "${_lib}")
        endif()
    endforeach()

    # Otherwise a wrong directory would silently ship the libraries unsigned.
    if(_file_list STREQUAL CPACK_ZEAL_EXECUTABLE)
        message(FATAL_ERROR "No libraries from ${CPACK_ZEAL_VCPKG_BIN_DIR} found in the package.")
    endif()
endif()

include(CodeSign)

foreach(_file ${_file_list})
    codesign(FILES "${CPACK_TEMPORARY_DIRECTORY}/${_file}" QUIET)
endforeach()
