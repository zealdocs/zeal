find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(session_test session_test.cpp)
target_link_libraries(session_test PRIVATE Core Qt6::Test)

zeal_add_test(session_test)

add_executable(extractor_test extractor_test.cpp)
target_link_libraries(extractor_test PRIVATE Core Qt6::Test)

find_package(LibArchive REQUIRED)
target_link_libraries(extractor_test PRIVATE LibArchive::LibArchive)

zeal_add_test(extractor_test)
