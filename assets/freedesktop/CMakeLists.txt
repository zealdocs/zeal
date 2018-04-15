if(UNIX AND NOT APPLE)
    find_package(ECM 1.0.0 REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(ECMInstallIcons)
    include(KDEInstallDirs)

    ecm_install_icons(ICONS "16-apps-zeal.png"
                            "24-apps-zeal.png"
                            "32-apps-zeal.png"
                            "64-apps-zeal.png"
                            "128-apps-zeal.png"
                      DESTINATION ${KDE_INSTALL_ICONDIR}
    )

    # TODO: Generate via zeal.desktop.in.
    install(FILES "zeal.desktop"
            DESTINATION ${KDE_INSTALL_APPDIR}
    )
endif()
