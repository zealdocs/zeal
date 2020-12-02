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

find_package(Qt5 COMPONENTS Concurrent Gui Network REQUIRED)
target_link_libraries(Registry Util Qt5::Concurrent Qt5::Gui Qt5::Network)
