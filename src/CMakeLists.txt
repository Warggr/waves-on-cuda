add_executable(waves main.cpp)
target_link_libraries(waves GL)
target_link_libraries(waves glfw)
target_link_libraries(waves GLEW::GLEW)
target_link_libraries(waves glm::glm)
target_link_libraries(waves GLU)

#find_package(Threads REQUIRED)
#target_link_libraries(waves Threads::Threads)
