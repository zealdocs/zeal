list(APPEND QxtGlobalShortcut_SOURCES
    qxtglobalshortcut.cpp
)

if(APPLE)
    list(APPEND QxtGlobalShortcut_SOURCES
        qxtglobalshortcut_mac.cpp
    )
elseif(UNIX)
    find_package(X11 QUIET COMPONENTS xcb xcb_keysyms xkbcommon)
    if(X11_xcb_FOUND AND X11_xcb_keysyms_FOUND AND X11_xkbcommon_FOUND)
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
elseif(UNIX AND X11_xcb_FOUND AND X11_xcb_keysyms_FOUND AND X11_xkbcommon_FOUND)
    target_link_libraries(QxtGlobalShortcut PRIVATE X11::xcb X11::xcb_keysyms X11::xkbcommon)
endif()
