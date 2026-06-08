find_package(Qt6 REQUIRED COMPONENTS Test)

# Fuzzy matching tests
add_executable(fuzzy_test fuzzy_test.cpp)
target_link_libraries(fuzzy_test PRIVATE Util Qt6::Test)

zeal_add_test(fuzzy_test)

# SQLite Statement tests
add_executable(statement_test statement_test.cpp)
target_link_libraries(statement_test PRIVATE Util Qt6::Test)

zeal_add_test(statement_test)

# Tarix archive tests
add_executable(tarixarchive_test tarixarchive_test.cpp)
target_link_libraries(tarixarchive_test PRIVATE Util Qt6::Test ZLIB::ZLIB)

zeal_add_test(tarixarchive_test)
