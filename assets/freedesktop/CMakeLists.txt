if(UNIX AND NOT APPLE)
    find_package(ECM 1.0.0 REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(ECMInstallIcons)

    if(QT_VERSION_MAJOR EQUAL 5)
        include(KDEInstallDirs)
    else()
        # Workaround until KDEInstallDirs6 is ready to use.
        include(GNUInstallDirs)
        set(KDE_INSTALL_APPDIR "${CMAKE_INSTALL_DATAROOTDIR}/applications")
        set(KDE_INSTALL_ICONDIR "${CMAKE_INSTALL_DATAROOTDIR}/icons")
        set(KDE_INSTALL_METAINFODIR "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
    endif()

    ecm_install_icons(ICONS "16-apps-zeal.png"
                            "24-apps-zeal.png"
                            "32-apps-zeal.png"
                            "64-apps-zeal.png"
                            "128-apps-zeal.png"
                      DESTINATION ${KDE_INSTALL_ICONDIR}
    )

# For development builds insert an extra release in the AppStream metadata.
if(NOT ZEAL_RELEASE_BUILD)
    string(TIMESTAMP ZEAL_APPSTREAM_DEV_RELEASE "\n    <release date=\"%Y-%m-%d\" version=\"${ZEAL_VERSION_FULL}\" type=\"development\" />")
endif()

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
