add_library(marching_cubes marching_cubes.cpp)
target_include_directories(marching_cubes PRIVATE "..")
target_link_libraries(marching_cubes viewer)
target_link_libraries(marching_cubes marching_cubes_constants)

add_executable(_3dviewer viewer.cpp)
target_link_libraries(_3dviewer marching_cubes)
