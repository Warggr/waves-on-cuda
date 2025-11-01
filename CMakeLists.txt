cmake_minimum_required(VERSION 3.12) # 3.12 required for C++20
project(waves_on_cuda VERSION 0.1.0 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 75)
endif()

find_package(glfw3 3.4 REQUIRED)
find_package(GLEW REQUIRED)

find_package(glm CONFIG)
if(NOT ${glm_FOUND})
include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)
endif()

add_subdirectory(scripts)
add_subdirectory(src)
add_subdirectory(res)
