#ifndef MYGLFW_H
#define MYGLFW_H

#include <glm/mat4x4.hpp>

class GLFWwindow;
class Grid;

class MyGLFW {
public:
    enum state {
        UNINITIALIZED, RUNNING, CLOSED
    };
private:
    state _state = UNINITIALIZED;
    GLFWwindow* window = nullptr;

    float yaw = -45.0f, pitch = 0;
    glm::vec3 cameraPos   = glm::vec3(-1.0f, 0.0f,  1.0f);
    glm::vec3 cameraFront = -cameraPos;
    glm::vec3 cameraUp    = glm::vec3(0.0f, 0.0f,  1.0f);
    float mouseX = 400, mouseY = 300;
    int width, height;
    bool first_mouse = true;
    std::size_t nbTriangles;

    void mouse_callback(double xpos, double ypos);
    friend void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    void processInput();
public:
    ~MyGLFW();
    void initialize();
    void render();
    void set_grid(const Grid* new_grid);
    void signal_should_close();
    bool closed();
};

#endif //MYGLFW_H
