#include "my_glfw.hpp"
#include "grid.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <cassert>
#include <cstring>

// Copied from https://codereview.stackexchange.com/a/22907
static std::vector<char> readAllBytes(const std::filesystem::path& filename) {
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    if(!ifs.is_open()){
        std::stringstream error_msg;
        error_msg << "File not found:" << filename;
        throw std::runtime_error(error_msg.str());
    }

    std::ifstream::pos_type pos = ifs.tellg();

    if (pos == 0) {
        return std::vector<char>{};
    }

    std::vector<char> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
}

std::string_view get_error_msg(GLenum error){
    switch(error){
        case GL_NO_ERROR: return "No error";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
        default: return "Unrecognized";
    }
}

struct Shader {
    unsigned int shader;

    Shader(const std::filesystem::path& filename, int shader_type, bool spirv = false) {
        std::vector<char> program = readAllBytes(filename);

        // Adapted from https://www.geeks3d.com/20200211/how-to-load-spir-v-shaders-in-opengl/
        // See also https://www.khronos.org/opengl/wiki/SPIR-V#Example
        assert(shader_type == GL_FRAGMENT_SHADER or shader_type == GL_VERTEX_SHADER);
        shader = glCreateShader(shader_type);
        if(shader == 0){
            std::stringstream error_msg;
            GLenum error = glGetError();
            error_msg << "Could not create shader: " << get_error_msg(error) << " (" << error << ")";
            throw std::runtime_error(error_msg.str());
        }
        if(spirv){
            glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, program.data(), program.size());
            glSpecializeShader(shader, "main", 0, nullptr, nullptr);
        } else {
            const char* source[1] = { program.data() };
            const GLint lengths[1] = { static_cast<int>(program.size()) };
            glShaderSource(shader, 1, source, lengths);
            glCompileShader(shader);
        }

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if(compiled == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());
            std::stringstream error_msg;
            error_msg << "Could not compile shader: " << std::string_view(infoLog.data(), infoLog.size()) << "(" << maxLength << " bytes log)";
            throw std::runtime_error(error_msg.str());
        }
    }
    ~Shader(){
        glDeleteShader(shader);
    }
    operator unsigned int () const { return shader; }
};

unsigned int load_shader_program(const std::filesystem::path &vertex_shader, std::filesystem::path fragment_shader) {
    const unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, Shader(vertex_shader, GL_VERTEX_SHADER, true));
    glAttachShader(shader_program, Shader(fragment_shader, GL_FRAGMENT_SHADER, true));
    glLinkProgram(shader_program);
    int success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        throw std::runtime_error("Could not link program");
    }
    return shader_program;
}

void error_callback(int error, const char* description){
    std::cerr << "Error: " << description << '\n';
}

void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << type << "," << id << "," << severity << "," << std::string_view(message, length) << '\n';
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    auto self = static_cast<MyGLFW*>(glfwGetWindowUserPointer(window));
    self->mouse_callback(xpos, ypos);
}

void MyGLFW::initialize() {
    if(!glfwInit()) {
        throw std::runtime_error("Could not init GLFW");
    }
    glfwSetErrorCallback(error_callback);
    window = glfwCreateWindow(640, 480, "Waves", nullptr, nullptr);
    if(!window) {
        throw std::runtime_error("Could not create window");
    }
    glfwMakeContextCurrent(window); // This needs to be before we initialize GLEW

    if(GLenum err = glewInit(); err != GLEW_OK) {
        std::stringstream err_msg;
        err_msg << "Could not initialize GLEW:" << glewGetErrorString(err);
        throw std::runtime_error(err_msg.str());
    }
    if(!GLEW_VERSION_2_1)
        throw std::runtime_error("Bad GLEW version");
    std::cerr << "Status: Using GLEW" << glewGetString(GLEW_VERSION) << '\n';
    glDebugMessageCallback(debug_callback, nullptr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, &::mouse_callback);

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    const unsigned int shader_program = load_shader_program("res/vertex_shader.spv", "res/fragment_shader.spv");
    glUseProgram(shader_program);

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    _state = RUNNING;
}

MyGLFW::~MyGLFW() {
    if (_state != UNINITIALIZED) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void MyGLFW::mouse_callback(double xpos, double ypos) {
    if (first_mouse) {
        first_mouse = false;
        mouseX = xpos; mouseY = ypos;
    }

    float xoffset = xpos - mouseX,
        yoffset = ypos - mouseY;
    mouseX = xpos; mouseY = ypos;
    constexpr float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;
    yaw -= xoffset;
    pitch -= yoffset;

    if(pitch > 89.0f)
        pitch =  89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(yaw) * cos(glm::radians(pitch)));
    direction.z = sin(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void MyGLFW::processInput() {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
        _state = CLOSED;
    } else if (glfwWindowShouldClose(window)) {
        _state = CLOSED;
    }

    const float cameraSpeed = 0.05f; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void MyGLFW::signal_should_close() {
    _state = CLOSED;
}

bool MyGLFW::closed() const {
    return _state == CLOSED;
}

void MyGLFW::set_grid(const Grid<double, 2>* grid_to_render) {
    GLfloat triangles[grid_to_render->shape()[0]-1][grid_to_render->shape()[1]-1][2][3][3];
    nbTriangles = sizeof(triangles) / sizeof(triangles[0][0][0][0]);

    const float grid_dx = 1.0 / static_cast<float>(grid_to_render->shape()[0]),
    grid_dy = 1.0 / static_cast<float>(grid_to_render->shape()[1]);
    for (int i = 1; i<grid_to_render->shape()[0]; i++) {
        for (int j = 1; j<grid_to_render->shape()[1]; j++) {
            const float topLeft[3] = { (i-1)*grid_dx, (j-1)*grid_dy, static_cast<float>((*grid_to_render)[i-1][j-1]) },
                topRight[3] = { (i-1)*grid_dx, j*grid_dy, static_cast<float>((*grid_to_render)[i-1][j]) },
                bottomLeft[3] = { i*grid_dx, (j-1)*grid_dy, static_cast<float>((*grid_to_render)[i][j-1]) },
                bottomRight[3] = { i*grid_dx, j*grid_dy, static_cast<float>((*grid_to_render)[i][j]) };

            auto& tr1 = triangles[i-1][j-1][0],
             &tr2 = triangles[i-1][j-1][1];

            std::memcpy(&tr1[0], &topLeft, sizeof(topLeft));
            std::memcpy(&tr1[1], &bottomLeft, sizeof(topLeft));
            std::memcpy(&tr1[2], &topRight, sizeof(topLeft));

            std::memcpy(&tr2[0], &topRight, sizeof(topLeft));
            std::memcpy(&tr2[1], &bottomLeft, sizeof(topLeft));
            std::memcpy(&tr2[2], &bottomRight, sizeof(topLeft));
        }
    }
    // Transfer points to GPU memory
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void MyGLFW::render() {
    processInput();
    glfwPollEvents();

    glm::mat4 proj[3] = {
        // Model transformation
        glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.01f)), glm::vec3(-0.5f, -0.5f, 0.0f)),
        // View
        glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp),
        // Projection
        glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 10.0f),
    };

    // Transfer shader params to GPU memory
    unsigned int ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(proj), nullptr, GL_DYNAMIC_DRAW); // Allocate memory

    // Upload data to UBO
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(proj), glm::value_ptr(proj[0]));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind the UBO to binding point 0
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, nbTriangles);
    glfwSwapBuffers(window);
}
