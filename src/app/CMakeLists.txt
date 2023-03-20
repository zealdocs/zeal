find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
if (Qt${QT_VERSION_MAJOR}Widgets_VERSION VERSION_LESS QT_MINIMUM_VERSION)
    message(FATAL_ERROR "Qt version >= ${QT_MINIMUM_VERSION} is required.")
endif()

# qt_standard_project_setup and qt_deploy_runtime_dependencies require Qt version >=6.3.0.
if(Qt${QT_VERSION_MAJOR}Widgets_VERSION VERSION_GREATER_EQUAL "6.3.0")
    set(use_qt63_cmake_commands TRUE)

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
    OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install Qt runtime dependencies on Windows.
if(WIN32 AND use_qt63_cmake_commands)
    # The following script will be executed at install time.
    set(qt_deploy_script "${CMAKE_CURRENT_BINARY_DIR}/qt-deploy-$<CONFIG>.cmake")

    file(GENERATE OUTPUT ${qt_deploy_script} CONTENT "
# Override deployment script's working directory.
set(QT_DEPLOY_PREFIX \"$<TARGET_FILE_DIR:App>\")

include(\"${QT_DEPLOY_SUPPORT}\")
qt_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE:App>\"
    BIN_DIR .
    VERBOSE
)")
endif()

if(APPLE)
    set_target_properties(App PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.zealdocs.zeal"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_ICON_FILE "zeal.icns"
        MACOSX_BUNDLE_COPYRIGHT ${PROJECT_COPYRIGHT}
        RESOURCE "resources/zeal.icns"
    )
elseif(WIN32 AND use_qt63_cmake_commands)
    # Install Qt runtime dependencies.
    install(SCRIPT ${qt_deploy_script})

    unset(qt_deploy_script)
    unset(use_qt63_cmake_commands)
elseif(UNIX)
    include(GNUInstallDirs)
    install(TARGETS App DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

#
# CPack Configuration.
#
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR ${PROJECT_COMPANY_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_VERBATIM_VARIABLES YES)

# Set options for the source package.
# Usage: cmake --build build --target package_source
set(CPACK_SOURCE_GENERATOR "TGZ;TXZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_OUTPUT_NAME}-${ZEAL_VERSION_FULL}")
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
