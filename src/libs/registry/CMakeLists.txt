add_library(Registry
    cancellationtoken.h
    docset.cpp
    docsetmetadata.cpp
    docsetregistry.cpp
    listmodel.cpp
    searchmodel.cpp
    searchquery.cpp
    searchresult.h # Only for Qt Creator to see it.
)

find_package(Qt5 COMPONENTS Concurrent Gui Network REQUIRED)
target_link_libraries(Registry Util Qt5::Concurrent Qt5::Gui Qt5::Network)
