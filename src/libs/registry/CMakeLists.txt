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

find_package(Qt6 COMPONENTS Concurrent Gui Network REQUIRED)
target_link_libraries(Registry Util Qt6::Concurrent Qt6::Gui Qt6::Network)
