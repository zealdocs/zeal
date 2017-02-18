list(APPEND QxtGlobalShortcut_SOURCES
    qxtglobalshortcut.cpp
)

if(APPLE)
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_mac.cpp
    )
elseif(UNIX)
    find_package(X11)
    if(X11_FOUND)
        list(APPEND QxtGlobalShortcut_SOURCES
            qxtglobalshortcut_x11.cpp
        )
    endif()
elseif(WIN32)
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_win.cpp
    )
endif()

add_library(QxtGlobalShortcut ${QxtGlobalShortcut_SOURCES})

find_package(Qt5Gui REQUIRED)
target_link_libraries(QxtGlobalShortcut Qt5::Gui)

if(APPLE)
    find_library(CARBON_LIBRARY Carbon)
    target_link_libraries(QxtGlobalShortcut ${CARBON_LIBRARY})
elseif(UNIX AND X11_FOUND)
    target_link_libraries(QxtGlobalShortcut ${X11_LIBRARIES})

    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
    target_link_libraries(QxtGlobalShortcut Qt5::X11Extras)

    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

    find_package(XCB COMPONENTS XCB KEYSYMS REQUIRED)
    target_link_libraries(QxtGlobalShortcut XCB::XCB XCB::KEYSYMS)
endif()
