cmake_minimum_required(VERSION 3.0.0)
project(waves_on_cuda VERSION 0.1.0 LANGUAGES CXX)

find_package(glfw3 3.4 REQUIRED)
find_package(GLEW REQUIRED)

add_subdirectory(src)
add_subdirectory(res)
