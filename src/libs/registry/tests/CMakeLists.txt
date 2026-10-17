find_package(Qt6 REQUIRED COMPONENTS Gui Test)

add_executable(docset_test docset_test.cpp)
target_link_libraries(docset_test PRIVATE Registry Util Qt6::Gui Qt6::Test)

zeal_add_test(docset_test)
