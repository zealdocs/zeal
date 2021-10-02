# Based on https://github.com/fakenmc/cmake-git-semver by Nuno Fachada.
# This module is public domain, use it as it fits you best.
#
# This cmake module sets the project version and partial version
# variables by analysing the git tag and commit history. It expects git
# tags defined with semantic versioning 2.0.0 (http://semver.org/).
#
# The module expects the PROJECT_NAME variable to be set, and recognizes
# the GIT_FOUND, GIT_EXECUTABLE and VERSION_UPDATE_FROM_GIT variables.
# If Git is found and VERSION_UPDATE_FROM_GIT is set to boolean TRUE,
# the project version will be updated using information fetched from the
# most recent git tag and commit. Otherwise, the module will try to read
# a VERSION file containing the full and partial versions. The module
# will update this file each time the project version is updated.
#
# Once done, this module will define the following variables:
#
# ${PROJECT_NAME}_GIT_VERSION_STRING - Version string without metadata
# such as "v2.0.0" or "v1.2.41-beta.1". This should correspond to the
# most recent git tag.
# ${PROJECT_NAME}_GIT_VERSION_STRING_FULL - Version string with metadata
# such as "v2.0.0+3.a23fbc" or "v1.3.1-alpha.2+4.9c4fd1"
# ${PROJECT_NAME}_GIT_VERSION_MAJOR - Major version integer (e.g. 2 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_GIT_VERSION_MINOR - Minor version integer (e.g. 3 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_GIT_VERSION_PATCH - Patch version integer (e.g. 1 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_GIT_VERSION_TWEAK - Tweak version string (e.g. "RC.2" in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_GIT_VERSION_AHEAD - How many commits ahead of last tag (e.g. 21 in v2.3.1-RC.2+21.ef12c8)
# ${PROJECT_NAME}_GIT_VERSION_SHA - The git sha1 of the most recent commit (e.g. the "ef12c8" in v2.3.1-RC.2+21.ef12c8)
# Only if VERSION_UPDATE_FROM_GIT is TRUE:
# ${PROJECT_NAME}_VERSION - Same as ${PROJECT_NAME}_GIT_VERSION_STRING,
# without the preceding 'v', e.g. "2.0.0" or "1.2.41-beta.1"

# Check if .git directory is present.
if(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
    message(NOTICE "Cannot find Git metadata, using static version string.")
    return()
endif()

# Check if Git executable is present.
find_package(Git)
if(NOT GIT_FOUND)
    message(NOTICE "Cannot find Git executable, using static version string.")
    return()
endif()

# Check if Git executable version is >= 2.15. Required for --is-shallow-repository argument.
# See https://stackoverflow.com/a/37533086.
if(GIT_VERSION_STRING VERSION_LESS "2.15")
    message(NOTICE "Git executable is too old (< 2.15), using static version string.")
    return()
endif()

# Detect shallow clone.
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --is-shallow-repository
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE IS_SHALLOW_RESULT
    OUTPUT_VARIABLE IS_SHALLOW_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(IS_SHALLOW_RESULT AND NOT IS_SHALLOW_RESULT EQUAL 0)
    message(NOTICE "Cannot perform shallow clone detection, using static version string.")
    unset(IS_SHALLOW_RESULT)
    unset(IS_SHALLOW_OUTPUT)
    return()
endif()

unset(IS_SHALLOW_RESULT)

if(NOT "${IS_SHALLOW_OUTPUT}" STREQUAL "false")
    message(NOTICE "Shallow clone detected, using static version string.")
    unset(IS_SHALLOW_OUTPUT)
    return()
endif()

unset(IS_SHALLOW_OUTPUT)

# Get last tag from git
execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_GIT_VERSION_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# How many commits since the last tag
execute_process(COMMAND ${GIT_EXECUTABLE} rev-list master ${${PROJECT_NAME}_GIT_VERSION_STRING}^..HEAD --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_GIT_VERSION_AHEAD
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get current commit SHA from git
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ${PROJECT_NAME}_GIT_VERSION_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get partial versions into a list
string(REGEX MATCHALL "-.*$|[0-9]+" ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    ${${PROJECT_NAME}_GIT_VERSION_STRING})

# Set the version numbers
list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    0 ${PROJECT_NAME}_GIT_VERSION_MAJOR)
list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    1 ${PROJECT_NAME}_GIT_VERSION_MINOR)
list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    2 ${PROJECT_NAME}_GIT_VERSION_PATCH)

# Calculate next patch version.
math(EXPR ${PROJECT_NAME}_GIT_VERSION_PATCH_NEXT ${${PROJECT_NAME}_GIT_VERSION_PATCH}+1)

# The tweak part is optional, so check if the list contains it
list(LENGTH ${PROJECT_NAME}_PARTIAL_VERSION_LIST
    ${PROJECT_NAME}_PARTIAL_VERSION_LIST_LEN)
if (${PROJECT_NAME}_PARTIAL_VERSION_LIST_LEN GREATER 3)
    list(GET ${PROJECT_NAME}_PARTIAL_VERSION_LIST 3 ${PROJECT_NAME}_GIT_VERSION_TWEAK)
    string(SUBSTRING ${${PROJECT_NAME}_GIT_VERSION_TWEAK} 1 -1 ${PROJECT_NAME}_GIT_VERSION_TWEAK)
endif()

# Unset the list
unset(${PROJECT_NAME}_PARTIAL_VERSION_LIST)

# Set full project version string
set(${PROJECT_NAME}_GIT_VERSION_STRING_FULL
    ${${PROJECT_NAME}_GIT_VERSION_STRING}+${${PROJECT_NAME}_GIT_VERSION_AHEAD}.${${PROJECT_NAME}_GIT_VERSION_SHA})

if(VERSION_UPDATE_FROM_GIT)
    # Set project version (without the preceding 'v')
    set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_GIT_VERSION_MAJOR}.${${PROJECT_NAME}_GIT_VERSION_MINOR}.${${PROJECT_NAME}_GIT_VERSION_PATCH})
    if (${PROJECT_NAME}_GIT_VERSION_TWEAK)
        set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION}-${${PROJECT_NAME}_GIT_VERSION_TWEAK})
    endif()
endif()
