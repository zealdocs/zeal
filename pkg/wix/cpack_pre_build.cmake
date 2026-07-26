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

# TODO: Automatically generate list.
set(_file_list
    "zeal.exe"
    "archive.dll"
    "zlib1.dll"
    "sqlite3.dll"
)

include(CodeSign)

foreach(_file ${_file_list})
    codesign(FILES "${CPACK_TEMPORARY_DIRECTORY}/${_file}" QUIET)
endforeach()
