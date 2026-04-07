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
elseif(UNIX AND X11_FOUND)
    target_link_libraries(QxtGlobalShortcut PRIVATE ${X11_LIBRARIES})

    if(Qt6Core_VERSION VERSION_GREATER_EQUAL 6.10)
        find_package(Qt6 COMPONENTS GuiPrivate REQUIRED)
    endif()
    target_link_libraries(QxtGlobalShortcut PRIVATE Qt6::GuiPrivate)

    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

    find_package(XCB COMPONENTS XCB KEYSYMS REQUIRED)
    target_link_libraries(QxtGlobalShortcut PRIVATE XCB::XCB XCB::KEYSYMS)
endif()
