find_package(Eigen3 REQUIRED NO_MODULE)
 
add_library(vof_scheme vof.cpp)
target_link_libraries(vof_scheme scheme)
target_link_libraries(vof_scheme Eigen3::Eigen)
