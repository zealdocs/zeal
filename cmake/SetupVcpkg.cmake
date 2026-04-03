#
# SetupVcpkg.cmake - Auto-detect and configure vcpkg integration
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#

function(setup_vcpkg)
    # Auto-detect vcpkg if available.
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        message(NOTICE "Using vcpkg from $ENV{VCPKG_ROOT}")
    endif()

    if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        return()
    endif()

    # Share installed packages across build configurations.
    if(NOT DEFINED CACHE{VCPKG_INSTALLED_DIR})
        set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/build/vcpkg_installed" CACHE STRING "")
    endif()

    # Default to release-only triplet on Windows to speed up builds.
    if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows-release" CACHE STRING "")
    endif()

    # Deploy vcpkg dependencies alongside the build output.
    if(NOT DEFINED X_VCPKG_APPLOCAL_DEPS_INSTALL)
        set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON CACHE BOOL "")
    endif()
endfunction()
