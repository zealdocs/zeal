#
# ZealHelpers.cmake - Project-specific CMake helpers
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#

# Register a unit test executable. On Windows, prepends Qt's DLL directory to
# PATH at test-run time so the test binary can locate Qt6Core.dll without
# deploying Qt next to every test executable.
#
# TODO: Once cmake_minimum_required is >= 3.27, replace the Qt-specific
# directory with $<TARGET_RUNTIME_DLL_DIRS:${test_name}> to cover any future
# non-Qt shared dependencies automatically.
function(zeal_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${test_name})
    if(WIN32)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()
endfunction()

# Attach the project's standard Qt precompiled-header set to a target. Every
# target receives <QObject> and <QString>; additional headers may be passed as
# extra arguments (e.g. zeal_attach_qt_pch(Ui <QDialog> <QWidget>)).
function(zeal_attach_qt_pch target)
    target_precompile_headers(${target} PRIVATE <QObject> <QString> ${ARGN})
endfunction()

# Read the version of a bundled dependency out of its header. `regex` must
# select the single line carrying it, e.g. "^#define CPPHTTPLIB_VERSION ".
function(zeal_bundled_version out_var header regex)
    file(STRINGS "${header}" line LIMIT_COUNT 1 REGEX "${regex}")
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" version "${line}")
    if(NOT version)
        message(FATAL_ERROR "Could not read a version matching '${regex}' from ${header}")
    endif()
    set(${out_var} "${version}" PARENT_SCOPE)
endfunction()
