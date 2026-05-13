#
# SetupVcpkg.cmake - Auto-detect and configure vcpkg integration
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#

function(setup_vcpkg)
    # Auto-detect vcpkg if not already configured.
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(_vcpkg_root "")
        if(DEFINED ENV{VCPKG_ROOT})
            if(EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
                set(_vcpkg_root "$ENV{VCPKG_ROOT}")
            else()
                message(WARNING
                    "VCPKG_ROOT is set to '$ENV{VCPKG_ROOT}' but does not contain a valid"
                    " vcpkg checkout (scripts/buildsystems/vcpkg.cmake is missing); ignoring."
                )
            endif()
        elseif(WIN32)
            # PATH fallback only on Windows, where vcpkg is the canonical dep source.
            # Other platforms use the system package manager and must not silently
            # engage vcpkg via a stray binary on PATH.
            # Resolve symlinks so e.g. /usr/local/bin/vcpkg -> /opt/vcpkg/vcpkg yields
            # /opt/vcpkg rather than /usr/local/bin.
            find_program(VCPKG_EXECUTABLE vcpkg)
            if(VCPKG_EXECUTABLE)
                get_filename_component(_vcpkg_real "${VCPKG_EXECUTABLE}" REALPATH)
                get_filename_component(_candidate "${_vcpkg_real}" DIRECTORY)
                if(EXISTS "${_candidate}/scripts/buildsystems/vcpkg.cmake")
                    set(_vcpkg_root "${_candidate}")
                endif()
            endif()
        endif()
        if(_vcpkg_root)
            set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
            message(NOTICE "Using vcpkg from ${_vcpkg_root}")
        endif()
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
