find_package(GTest)

enable_testing()

add_executable(
    test
    test_grid.cpp
    test_vof.cpp
)

target_link_libraries(
    test
    GTest::gtest_main
    vof_scheme
    alloc
)

target_include_directories(
    test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

include(GoogleTest)
gtest_discover_tests(test)
