add_executable(waves main.cpp grid.cu)
target_link_libraries(waves GL)
target_link_libraries(waves glfw)
target_link_libraries(waves GLEW::GLEW)
target_link_libraries(waves glm::glm)
target_link_libraries(waves GLU)

if(APPLE)
    # We need to add the path to the driver (libcuda.dylib) as an rpath,
    # so that the static cuda runtime can find it at runtime.
    set_property(TARGET waves
            PROPERTY
            BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()

#find_package(Threads REQUIRED)
#target_link_libraries(waves Threads::Threads)
