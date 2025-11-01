add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/marching_cubes_cache.hpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/structures.py"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py" h -o "${CMAKE_CURRENT_BINARY_DIR}/generated/marching_cubes_cache.hpp"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cache.cpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/cache.cpp.jinja" "${CMAKE_CURRENT_SOURCE_DIR}/structures.py"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py" c -o "${CMAKE_CURRENT_BINARY_DIR}/generated/cache.cpp"
)

add_custom_target(generate_cache ALL
    DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/generated/marching_cubes_cache.hpp"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/cache.cpp"
)

add_library(marching_cubes_constants "${CMAKE_CURRENT_BINARY_DIR}/generated/cache.cpp")
add_dependencies(marching_cubes_constants generate_cache)
target_include_directories(marching_cubes_constants PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
