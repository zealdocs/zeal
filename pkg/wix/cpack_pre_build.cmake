if(CPACK_SOURCE_INSTALLED_DIRECTORIES)
    message(DEBUG "Skipping package signing for source package generator.")
    return()
endif()

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
