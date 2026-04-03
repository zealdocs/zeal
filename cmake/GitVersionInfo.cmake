#
# GitVersionInfo.cmake - Collect version-related information from Git
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# This module checks whether the current build is from a tagged commit
# or from a development branch ahead of a tag.
#
# Once done, this module will define the following variables:
#
# ${PROJECT_NAME}_GIT_AT_TAG - TRUE if HEAD is exactly at a tag
# ${PROJECT_NAME}_GIT_VERSION_AHEAD - Number of commits ahead of the last tag

function(_git_version_info)
    # Check if .git directory is present.
    if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
        return()
    endif()

    # Check if Git executable is present.
    find_package(Git)
    if(NOT GIT_FOUND)
        return()
    endif()

    # Require Git >= 2.15 for --is-shallow-repository.
    if(GIT_VERSION_STRING VERSION_LESS "2.15")
        return()
    endif()

    # Detect shallow clone.
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --is-shallow-repository
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if(_result AND NOT _result EQUAL 0)
        return()
    endif()

    if(NOT "${_output}" STREQUAL "false")
        return()
    endif()

    # Check if HEAD is at a tag.
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _result
        OUTPUT_QUIET ERROR_QUIET)

    if(_result EQUAL 0)
        set(${PROJECT_NAME}_GIT_AT_TAG TRUE PARENT_SCOPE)
        return()
    endif()

    # Get last tag.
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _last_tag
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if(_result AND NOT _result EQUAL 0)
        return()
    endif()

    # Count commits since last tag.
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${_last_tag}..HEAD --count
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _ahead
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if(_result AND NOT _result EQUAL 0)
        return()
    endif()

    set(${PROJECT_NAME}_GIT_VERSION_AHEAD ${_ahead} PARENT_SCOPE)
endfunction()

_git_version_info()
