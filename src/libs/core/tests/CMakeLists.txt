find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(state_test state_test.cpp)
target_link_libraries(state_test PRIVATE Core Qt6::Test)

zeal_add_test(state_test)
