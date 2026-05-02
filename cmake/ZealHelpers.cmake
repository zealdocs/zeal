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
