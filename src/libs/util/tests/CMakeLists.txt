find_package(Qt6 REQUIRED COMPONENTS Test)

# Fuzzy matching tests
add_executable(fuzzy_test fuzzy_test.cpp)
target_link_libraries(fuzzy_test PRIVATE Util Qt6::Test)

add_test(NAME fuzzy_test COMMAND fuzzy_test)
