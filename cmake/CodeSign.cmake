#
# CodeSign.cmake - CMake helper for signing Windows executables
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#

include_guard()

# codesign(FILES <files>...
#          [DESCRIPTION] <description-string>
#          [URL] <url-string>
#          [CERTIFICATE_FILE] <filename>
#          [PASSWORD] <password-string>
#          [TIMESTAMP_URL] <url-string>
#          [QUIET]
#          [VERBOSE]
#          [DEBUG])
function(codesign)
    # Cleans up temporary files created during signing.
    macro(_cleanup)
        if(DEFINED _certificate_file)
            file(REMOVE ${_certificate_file})
        endif()
    endmacro()

    # Sets '_certificate_file' variable to a temporary file path.
    macro(_set_temporary_certificate_file)
        # Determine temporary file location. Try to keep it local to the build.
        if(CMAKE_BINARY_DIR)
            set(_temp_path ${CMAKE_BINARY_DIR})
        elseif(CPACK_TEMPORARY_DIRECTORY)
            set(_temp_path ${CPACK_TEMPORARY_DIRECTORY})
        else()
            set(_temp_path $ENV{TEMP})
        endif()

        set(_certificate_file "${_temp_path}/codesign.tmp")

        # Remove file if left from previous run.
        _cleanup()
    endmacro()

    if(NOT WIN32)
        message(FATAL_ERROR "Code signing is only supported on Windows.")
    endif()

    cmake_parse_arguments(_ARG
        "QUIET;VERBOSE;DEBUG"                                     # Options.
        "DESCRIPTION;URL;CERTIFICATE_FILE;PASSWORD;TIMESTAMP_URL" # Single-value keywords.
        "FILES"                                                   # Multi-value keywords.
        ${ARGN}
    )

    if(NOT _ARG_FILES)
        message(FATAL_ERROR "FILES argument is required.")
    endif()

    # Find signtool executable.
    # TODO: Add option for path to signtool.exe.

    # Add Windows 10 SDK paths.
    get_filename_component(_w10sdk_root_path
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
        ABSOLUTE CACHE
    )
    if(_w10sdk_root_path)
        file(GLOB _w10sdk_paths "${_w10sdk_root_path}/bin/10.*")
        list(REVERSE _w10sdk_paths) # Newest version first.

        # Detect target architecture.
        # https://learn.microsoft.com/en-us/windows/win32/winprog64/wow64-implementation-details#environment-variables
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
            set(_w10sdk_arch "x64")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "X86")
            set(_w10sdk_arch "x86")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(_w10sdk_arch "arm64")
        else()
            message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}.")
        endif()
    endif()

    # TODO: Add microsoft.windows.sdk.buildtools path.

    find_program(_cmd
        NAMES signtool
        PATHS ${_w10sdk_paths}
        PATH_SUFFIXES ${_w10sdk_arch}
    )

    if(NOT _cmd)
        message(NOTICE "signtool.exe was not found, no binaries will be signed.")
        return()
    endif()

    message(DEBUG "Found signtool.exe: ${_cmd}")

    # Start constructing command.
    set(_cmd_args "sign")
    list(APPEND _cmd_args "/fd" "sha256")

    # Set certificate file.
    if(NOT _ARG_CERTIFICATE_FILE)
        if(CODESIGN_CERTIFICATE_FILE)
            if(NOT EXISTS ${CODESIGN_CERTIFICATE_FILE})
                message(NOTICE "Certificate file '${CODESIGN_CERTIFICATE_FILE}' does not exist.")
                return()
            endif()

            set(_ARG_CERTIFICATE_FILE ${CODESIGN_CERTIFICATE_FILE})
        elseif(DEFINED ENV{CODESIGN_CERTIFICATE_FILE})
            if("$ENV{CODESIGN_CERTIFICATE_FILE}" STREQUAL "")
                message(NOTICE "CODESIGN_CERTIFICATE_FILE is set to an empty string.")
                return()
            endif()

            if(NOT EXISTS $ENV{CODESIGN_CERTIFICATE_FILE})
                message(NOTICE "Certificate file '$ENV{CODESIGN_CERTIFICATE_FILE}' (set in CODESIGN_CERTIFICATE_FILE) does not exist.")
                return()
            endif()

            set(_ARG_CERTIFICATE_FILE $ENV{CODESIGN_CERTIFICATE_FILE})
        elseif(DEFINED ENV{CODESIGN_CERTIFICATE})
            if("$ENV{CODESIGN_CERTIFICATE}" STREQUAL "")
                message(NOTICE "CODESIGN_CERTIFICATE is set to an empty string.")
                return()
            endif()

            # Store certificate value in a temporary file for signtool to use.
            _set_temporary_certificate_file()
            file(WRITE ${_certificate_file} $ENV{CODESIGN_CERTIFICATE})
            set(_ARG_CERTIFICATE_FILE ${_certificate_file})
        elseif(DEFINED ENV{CODESIGN_CERTIFICATE_BASE64})
            if("$ENV{CODESIGN_CERTIFICATE_BASE64}" STREQUAL "")
                message(NOTICE "CODESIGN_CERTIFICATE_BASE64 is set to an empty string.")
                return()
            endif()

            # Read base64-encoded certificate from environment variable,
            # decode with `certutil.exe`, and store in a temporary file
            # for signtool to use.
            #
            # This is useful for GitHub Actions, which cannot handle unencoded
            # multiline secrets.

            _set_temporary_certificate_file()

            # Save base64-encoded certificate to file.
            set(_certificate_base64_file "${_certificate_file}.base64")
            file(WRITE ${_certificate_base64_file} $ENV{CODESIGN_CERTIFICATE_BASE64})

            # Decode certificate.
            set(_cmd_certutil_args "-decode" ${_certificate_base64_file} ${_certificate_file})
            execute_process(COMMAND "certutil.exe" ${_cmd_certutil_args}
                RESULT_VARIABLE _rc
                OUTPUT_VARIABLE _stdout
                # ERROR_VARIABLE  _stderr
            )

            # Remove temporary file first.
            file(REMOVE ${_certificate_base64_file})

            if(NOT _rc EQUAL 0)
                # For some reason certutil prints errors to stdout.
                message(WARNING "Failed to decode certificate: ${_stdout}")
                _cleanup()
                return()
            endif()

            set(_ARG_CERTIFICATE_FILE ${_certificate_file})
        else()
            message(NOTICE "Certificate is not provided, no binaries will be signed.")
            return()
        endif()
    endif()

    list(APPEND _cmd_args "/f" ${_ARG_CERTIFICATE_FILE})

    # Set password.
    if(NOT _ARG_PASSWORD)
        if(CODESIGN_PASSWORD)
            set(_ARG_PASSWORD ${CODESIGN_PASSWORD})
        elseif(DEFINED ENV{CODESIGN_PASSWORD})
            if("$ENV{CODESIGN_PASSWORD}" STREQUAL "")
                message(NOTICE "CODESIGN_PASSWORD is set to an empty string. Unset if not used.")
                _cleanup()
                return()
            endif()

            set(_ARG_PASSWORD $ENV{CODESIGN_PASSWORD})
        endif()
    endif()

    if(_ARG_PASSWORD)
        list(APPEND _cmd_args "/p" ${_ARG_PASSWORD})
    endif()

    # Set description.
    if(NOT _ARG_DESCRIPTION AND PROJECT_DESCRIPTION)
        set(_ARG_DESCRIPTION ${PROJECT_DESCRIPTION})
    endif()

    if(_ARG_DESCRIPTION)
        list(APPEND _cmd_args "/d" ${_ARG_DESCRIPTION})
    endif()

    # Set project URL.
    if(NOT _ARG_URL AND PROJECT_HOMEPAGE_URL)
        set(_ARG_URL ${PROJECT_HOMEPAGE_URL})
    endif()

    if(_ARG_URL)
        list(APPEND _cmd_args "/du" ${_ARG_URL})
    endif()

    # Set timestamp server.
    if(NOT _ARG_TIMESTAMP_URL)
        set(_ARG_TIMESTAMP_URL "http://timestamp.digicert.com")
    endif()

    if(_ARG_TIMESTAMP_URL)
        list(APPEND _cmd_args "/t" ${_ARG_TIMESTAMP_URL})
    endif()

    # Set quiet, verbose, or debug options.
    if(_ARG_QUIET)
        list(APPEND _cmd_args "/q")
    endif()

    if(_ARG_VERBOSE)
        list(APPEND _cmd_args "/v")
    endif()

    if(_ARG_DEBUG)
        list(APPEND _cmd_args "/debug")
    endif()

    foreach(_file ${_ARG_FILES})
        if(NOT EXISTS ${_file})
            message(NOTICE "Cannot find file to sign: ${_file}")
            continue()
        endif()

        if(_rc EQUAL 0)
            message(STATUS "Successfully signed: ${_file}")
        else()
            message(NOTICE "Failed to sign: ${_stderr}")

            if(NOT _ARG_QUIET)
                message(VERBOSE ${_stdout})
            endif()
        endif()
    endforeach()

    _cleanup()
endfunction()
