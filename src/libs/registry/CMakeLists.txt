add_library(Registry STATIC
    docset.cpp
    docsetmetadata.cpp
    docsetregistry.cpp
    listmodel.cpp
    searchmodel.cpp
    searchquery.cpp

    # Show headers without .cpp in Qt Creator.
    cancellationtoken.h
    itemdatarole.h
    searchresult.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent Gui Network REQUIRED)
target_link_libraries(Registry Util Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network)
