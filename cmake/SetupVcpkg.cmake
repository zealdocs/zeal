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

    # On Windows, default to a release-only triplet to speed up builds. Switch to the
    # default (debug+release) triplet only for true Debug builds — RelWithDebInfo and
    # MinSizeRel still link against release-built deps. Multi-config generators (Visual
    # Studio, Ninja Multi-Config, Xcode) leave CMAKE_BUILD_TYPE empty at configure time,
    # so default to the full triplet to cover every config the user may build.
    if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
        get_cmake_property(_vcpkg_is_multi_config GENERATOR_IS_MULTI_CONFIG)
        if(_vcpkg_is_multi_config OR CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-windows-release" CACHE STRING "")
        endif()
        unset(_vcpkg_is_multi_config)
    endif()

    # Deploy vcpkg dependencies alongside the build output.
    if(NOT DEFINED X_VCPKG_APPLOCAL_DEPS_INSTALL)
        set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON CACHE BOOL "")
    endif()
endfunction()
