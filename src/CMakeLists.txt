add_library(viewer my_glfw.cpp grid.cu)
target_link_libraries(viewer PUBLIC GL)
target_link_libraries(viewer PRIVATE glfw)
target_link_libraries(viewer PRIVATE GLEW::GLEW)
target_link_libraries(viewer PUBLIC glm::glm)
target_link_libraries(viewer PRIVATE GLU)
target_include_directories(viewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(NO_CUDA "Do not use CUDA." off)

if(NO_CUDA)
    add_compile_definitions(viewer NO_CUDA)
endif()

add_subdirectory(marching_cubes)

if(APPLE)
    # We need to add the path to the driver (libcuda.dylib) as an rpath,
    # so that the static cuda runtime can find it at runtime.
    set_property(TARGET viewer
            PROPERTY
            BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()

#find_package(Threads REQUIRED)
#target_link_libraries(waves Threads::Threads)

add_executable(waves main.cpp)

find_package(Boost 1.40 COMPONENTS program_options REQUIRED)
target_link_libraries(waves Boost::program_options)
target_link_libraries(waves viewer)
