find_package(Qt6 REQUIRED COMPONENTS Test)

# Fuzzy matching tests
add_executable(fuzzy_test fuzzy_test.cpp)
target_link_libraries(fuzzy_test PRIVATE Util Qt6::Test)

zeal_add_test(fuzzy_test)
