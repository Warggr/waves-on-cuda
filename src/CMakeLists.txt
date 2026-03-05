add_library(scheme INTERFACE)
target_include_directories(scheme INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(viewer my_glfw.cpp)
target_link_libraries(viewer PUBLIC GL)
target_link_libraries(viewer PRIVATE glfw)
target_link_libraries(viewer PRIVATE GLEW::GLEW)
target_link_libraries(viewer PUBLIC glm::glm)
target_link_libraries(viewer PRIVATE GLU)
target_include_directories(viewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(advection_scheme advection_scheme.cu)
target_include_directories(advection_scheme PUBLIC scheme)


option(NO_CUDA "Do not use CUDA." off)

add_library(alloc grid.cu)
if(NO_CUDA)
    target_compile_definitions(alloc PUBLIC NO_CUDA)
else()
endif()

add_subdirectory(marching_cubes)
add_subdirectory(vof)

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
target_link_libraries(waves mc_renderer)
target_link_libraries(waves vof_scheme)
target_link_libraries(waves viewer)

option(NUMPY_LOAD "Load initial conditions from .npy files" on)

if(NUMPY_LOAD)
    include(FetchContent)
    FetchContent_Declare(cnpy
        GIT_REPOSITORY "https://github.com/Warggr/cnpy"
        GIT_SHALLOW True
    )
    FetchContent_MakeAvailable(cnpy)
    add_library(npz_loader npz_loader.cpp)
    target_link_libraries(npz_loader cnpy)
    target_link_libraries(waves npz_loader)
    target_compile_definitions(waves PRIVATE NUMPY_LOAD)
endif()
