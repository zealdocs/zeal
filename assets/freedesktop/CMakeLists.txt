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

    configure_file(
        org.zealdocs.zeal.appdata.xml.in
        org.zealdocs.zeal.appdata.xml
    )

    install(FILES ${CMAKE_BINARY_DIR}/assets/freedesktop/org.zealdocs.zeal.appdata.xml
            DESTINATION ${KDE_INSTALL_METAINFODIR}
    )

    install(FILES "org.zealdocs.zeal.desktop"
            DESTINATION ${KDE_INSTALL_APPDIR}
    )
endif()
