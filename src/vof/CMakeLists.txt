find_package(Eigen3 REQUIRED NO_MODULE)
 
add_library(vof_scheme SHARED vof.cpp)
target_link_libraries(vof_scheme PUBLIC scheme)
target_link_libraries(vof_scheme PRIVATE alloc)
target_link_libraries(vof_scheme PRIVATE Eigen3::Eigen)
