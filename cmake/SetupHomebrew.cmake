#
# SetupHomebrew.cmake - Add Homebrew prefix to CMAKE_PREFIX_PATH on macOS
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# Makes Homebrew-installed dependencies discoverable via standard find_package()
# without manual HINTS or per-call fallback path lookups.
#

function(setup_homebrew)
    if(NOT APPLE)
        return()
    endif()

    # If a vcpkg toolchain is active, defer to it — its triplet root drives search.
    if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        return()
    endif()

    # Prefer `brew --prefix` (handles non-standard installs); fall back to
    # the well-known paths per host architecture.
    find_program(_brew_executable brew)
    if(_brew_executable)
        execute_process(
            COMMAND "${_brew_executable}" --prefix
            OUTPUT_VARIABLE _brew_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _brew_rc
        )
        if(_brew_rc EQUAL 0 AND IS_DIRECTORY "${_brew_prefix}")
            set(_homebrew_prefix "${_brew_prefix}")
        endif()
    endif()

    if(NOT _homebrew_prefix)
        if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" AND IS_DIRECTORY "/opt/homebrew")
            set(_homebrew_prefix "/opt/homebrew")
        elseif(IS_DIRECTORY "/usr/local/Homebrew")
            set(_homebrew_prefix "/usr/local")
        endif()
    endif()

    if(NOT _homebrew_prefix)
        return()
    endif()

    list(PREPEND CMAKE_PREFIX_PATH "${_homebrew_prefix}")

    # libarchive is keg-only on Homebrew (macOS ships a stale version);
    # add its prefix so find_package(LibArchive) finds it without manual hints.
    if(IS_DIRECTORY "${_homebrew_prefix}/opt/libarchive")
        list(PREPEND CMAKE_PREFIX_PATH "${_homebrew_prefix}/opt/libarchive")
    endif()

    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
    message(NOTICE "Using Homebrew prefix ${_homebrew_prefix}")
endfunction()
