find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

# Fuzzy matching tests
add_executable(fuzzy_test fuzzy_test.cpp)
target_link_libraries(fuzzy_test PRIVATE Util Qt::Test)

add_test(NAME fuzzy_test COMMAND fuzzy_test)
