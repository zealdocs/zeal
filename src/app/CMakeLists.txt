find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent WebEngineWidgets Widgets REQUIRED)
if (Qt${QT_VERSION_MAJOR}Widgets_VERSION VERSION_LESS QT_MINIMUM_VERSION)
    message(FATAL_ERROR "Qt version >= ${QT_MINIMUM_VERSION} is required.")
endif()

# Define output binary name.
if(APPLE)
    set(_project_output_name ${CMAKE_PROJECT_NAME})
else()
    string(TOLOWER ${CMAKE_PROJECT_NAME} _project_output_name)
endif()

set(PROJECT_EXECUTABLE_NAME "${_project_output_name}${CMAKE_EXECUTABLE_SUFFIX}")
message(STATUS "Project executable name: ${PROJECT_EXECUTABLE_NAME}")

# Only support installing runtime dependencies with Qt >=6.5.1 (see QTBUG-111741).
if(Qt${QT_VERSION_MAJOR}Widgets_VERSION VERSION_GREATER_EQUAL "6.5.1")
    set(_use_qt_cmake_commands TRUE)

    qt_standard_project_setup()
endif()

if(APPLE)
    list(APPEND App_RESOURCES resources/zeal.icns)
elseif(WIN32)
    configure_file(versioninfo.rc.in ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
    list(APPEND App_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
else()
    set(App_RESOURCES) # Silence CMake warning.
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(App WIN32
        main.cpp
        zeal.qrc
        ${App_RESOURCES}
    )
else()
    add_executable(App WIN32
        main.cpp
        zeal.qrc
        ${App_RESOURCES}
    )
endif()

target_link_libraries(App PRIVATE Core Util Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(App PROPERTIES
    OUTPUT_NAME ${_project_output_name}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Install Qt runtime dependencies on Windows.
if(WIN32 AND _use_qt_cmake_commands)
    qt_generate_deploy_script(
        TARGET App
        OUTPUT_SCRIPT _qt_deploy_script
        CONTENT "
# TODO: Run windeployqt after build.
# Override deployment script's working directory.
# set(QT_DEPLOY_PREFIX \"$<TARGET_FILE_DIR:App>\")

qt_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE:App>\"
    BIN_DIR .
    NO_TRANSLATIONS
    NO_COMPILER_RUNTIME
)")
endif()

if(APPLE)
    set_target_properties(App PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.zealdocs.zeal"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_ICON_FILE "zeal.icns"
        MACOSX_BUNDLE_COPYRIGHT ${PROJECT_COPYRIGHT}
        RESOURCE "resources/zeal.icns"
    )
elseif(WIN32)
    install(TARGETS App RUNTIME DESTINATION .)

    if(_use_qt_cmake_commands)
        # Install Qt runtime dependencies.
        install(SCRIPT ${_qt_deploy_script})

        unset(_qt_deploy_script)
        unset(_use_qt_cmake_commands)
    endif()
elseif(UNIX)
    include(GNUInstallDirs)
    install(TARGETS App DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

#
# CPack configuration.
#
set(CPACK_GENERATOR "7Z;ZIP")

set(CPACK_VERBATIM_VARIABLES YES)

# Usage: cmake --build --preset <preset-name> --target package
# E.g. cmake --build build --preset ninja-multi-vcpkg-release --target package
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR ${PROJECT_COMPANY_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/src/app/resources/zeal.ico")

# Set binary package file name.
if(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(_package_file_name_suffix "-windows-x64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "IA64")
        set(_package_file_name_suffix "-windows-x64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "X86")
        set(_package_file_name_suffix "-windows-x86")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        set(_package_file_name_suffix "-windows-arm64")
    else()
        set(_package_file_name_suffix "-windows-unknown")
    endif()
else()
    set(_package_file_name_suffix "")
endif()

if(ZEAL_PORTABLE_BUILD)
    string(PREPEND _package_file_name_suffix "-portable")
endif()

set(CPACK_PACKAGE_FILE_NAME "${_project_output_name}-${ZEAL_VERSION_FULL}${_package_file_name_suffix}")

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_PACKAGE_EXECUTABLES ${_project_output_name} ${CPACK_PACKAGE_NAME})
set(CPACK_CREATE_DESKTOP_LINKS ${_project_output_name} ${CPACK_PACKAGE_NAME})

# Allow CPack to do text to RTF conversion.
configure_file("${CMAKE_SOURCE_DIR}/COPYING" "${CMAKE_CURRENT_BINARY_DIR}/license.txt" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/license.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_PACKAGE_CHECKSUM SHA256)

if(WIN32)
    # CPack WiX configuration.
    set(CPACK_WIX_UPGRADE_GUID "5C4B6030-A1B4-4EFE-A5AF-28F6FA2E7978")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT ${CMAKE_PROJECT_HOMEPAGE_URL})
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/src/app/resources/zeal.ico")
    #set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/pkg/wix/banner.png")
    #set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/pkg/wix/dialog.png")
    set(CPACK_WIX_EXTENSIONS "WixUtilExtension.dll")
    set(CPACK_WIX_UI_REF "Zeal_InstallDir")
    set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/pkg/wix/template.xml")
    set(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/pkg/wix/patch.xml")
    set(CPACK_WIX_EXTRA_SOURCES
        "${CMAKE_SOURCE_DIR}/pkg/wix/ui.wxs"
        "${CMAKE_SOURCE_DIR}/pkg/wix/exitdialog.wxs"
    )

    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/pkg/wix/cpack_pre_build.cmake")

    if(NOT ZEAL_PORTABLE_BUILD)
        list(APPEND CPACK_GENERATOR "WIX")
        set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/pkg/wix/cpack_post_build.cmake")
    endif()
endif()

# Set options for the source package.
# Usage: cmake --build <build> --target package_source
set(CPACK_SOURCE_GENERATOR "TGZ;TXZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${_project_output_name}-${ZEAL_VERSION_FULL}")
set(CPACK_SOURCE_IGNORE_FILES
    # Directories.
    ".git/"
    ".github/"
    ".vscode/"
    "build/"
    # Files.
    ".editorconfig"
    ".gitattributes"
    ".gitignore"
)

include(CPack)
