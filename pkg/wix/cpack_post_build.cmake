if(CPACK_SOURCE_INSTALLED_DIRECTORIES)
    message(DEBUG "Skipping package signing for source package generator.")
    return()
endif()

if(NOT CPACK_GENERATOR STREQUAL "WIX")
    message(DEBUG "Skipping package signing for ${CPACK_GENERATOR} generator.")
    return()
endif()

include(CodeSign)
codesign(FILES ${CPACK_PACKAGE_FILES} QUIET)
