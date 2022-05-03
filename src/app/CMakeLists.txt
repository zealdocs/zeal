if(APPLE)
    list(APPEND App_RESOURCES resources/zeal.icns)
elseif(WIN32)
    configure_file(versioninfo.rc.in ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
    list(APPEND App_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
endif()

add_executable(App WIN32
    main.cpp
    zeal.qrc
    ${App_RESOURCES}
)

target_link_libraries(App Core Util)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
if (Qt6Widgets_VERSION VERSION_LESS QT_MINIMUM_VERSION)
    message(FATAL_ERROR "Qt version >= ${QT_MINIMUM_VERSION} is required.")
endif()

target_link_libraries(App Qt6::Widgets)

set_target_properties(App PROPERTIES
    OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

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
elseif(UNIX)
    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_KDE_MODULE_DIR})
    include(KDEInstallDirs)

    install(TARGETS App DESTINATION ${KDE_INSTALL_BINDIR})
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
