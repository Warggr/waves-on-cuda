find_package(GTest)

enable_testing()

add_executable(
    test-grid
    test_grid.cpp
    test_vof.cpp
)

target_link_libraries(
    test-grid
    GTest::gtest_main
    vof_scheme
    alloc
)

target_include_directories(
    test-grid PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

include(GoogleTest)
gtest_discover_tests(test-grid)

option(USE_PYTESTS "Build Python-based tests. Requires Python and PyBind11." on)

if(USE_PYTESTS)
    find_package(Python COMPONENTS Interpreter Development)
    find_package(pybind11 CONFIG)
    execute_process(COMMAND pytest-cmake --cmake-dir OUTPUT_VARIABLE pytest_DIR)
    find_package(Pytest REQUIRED)
    pybind11_add_module(vof_bindings MODULE vof_bindings.cpp)
    target_link_libraries(vof_bindings PRIVATE vof_scheme)

    pytest_discover_tests(
        python-vof-tests
        PYTHON_PATH_PREPEND ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS vof_bindings
    )
endif()
