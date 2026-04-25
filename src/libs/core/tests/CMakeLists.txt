find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(session_test session_test.cpp)
target_link_libraries(session_test PRIVATE Core Qt6::Test)

zeal_add_test(session_test)
