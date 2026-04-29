list(APPEND QxtGlobalShortcut_SOURCES
    qxtglobalshortcut.cpp
)

if(APPLE)
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_mac.cpp
    )
elseif(UNIX)
    # ECM provides FindXCB.cmake; neither CMake nor libxcb ships one.
    find_package(ECM QUIET NO_MODULE)
    if(ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})
        find_package(XCB COMPONENTS XCB KEYSYMS QUIET)
    endif()

    if(XCB_FOUND)
        list(APPEND QxtGlobalShortcut_SOURCES
            qxtglobalshortcut_x11.cpp
        )
    else()
        list(APPEND QxtGlobalShortcut_SOURCES
            qxtglobalshortcut_noop.cpp
        )
    endif()
elseif(WIN32)
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_win.cpp
    )
else()
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_noop.cpp
    )
endif()

add_library(QxtGlobalShortcut STATIC ${QxtGlobalShortcut_SOURCES})

find_package(Qt6 COMPONENTS Gui REQUIRED)
target_link_libraries(QxtGlobalShortcut PRIVATE Qt6::Gui)

if(APPLE)
    find_library(CARBON_LIBRARY Carbon)
    target_link_libraries(QxtGlobalShortcut PRIVATE ${CARBON_LIBRARY})
elseif(UNIX AND XCB_FOUND)
    if(Qt6Core_VERSION VERSION_GREATER_EQUAL 6.10)
        find_package(Qt6 COMPONENTS GuiPrivate REQUIRED)
    endif()
    target_link_libraries(QxtGlobalShortcut PRIVATE Qt6::GuiPrivate XCB::XCB XCB::KEYSYMS)
endif()
