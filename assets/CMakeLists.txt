if(UNIX AND NOT APPLE)
    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_KDE_MODULE_DIR})
    include(KDEInstallDirs)

    foreach(_i 16 24 32 64 128)
        install(FILES "freedesktop/appicons/${_i}/zeal.png"
            DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/${_i}x${_i}/apps"
            # TODO: Use `RENAME zeal.png` and get rid of subdirectories.
        )
    endforeach()

    # TODO: Generate via zeal.desktop.in.
    install(FILES "freedesktop/zeal.desktop"
        DESTINATION ${KDE_INSTALL_APPDIR}
    )
endif()
