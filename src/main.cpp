#include "grid.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <filesystem>
#include <mutex>
#include <vector>
#include <fstream>
#include <sstream>
#include <string_view>
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

class MyGLFW {
    GLFWwindow* window = nullptr;
    std::thread ui_thread;
    std::mutex mutex;
    const Grid* grid_to_render = nullptr; // Protected by the mutex
    bool should_exit = false; // Also protected by the mutex
public:
    MyGLFW() {
        ui_thread = std::thread([this](){ this->thread_main(); });
    }
    ~MyGLFW() {
        if(ui_thread.joinable()){
            {
                std::lock_guard guard(mutex);
                should_exit = true;
            }
            ui_thread.join();
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void thread_main() {
        if(!glfwInit()) {
            throw std::runtime_error("Could not init GLFW");
        }
        glfwSetErrorCallback(MyGLFW::error_callback);
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
        glDebugMessageCallback(MyGLFW::debug_callback, nullptr);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        const unsigned int shader_program = load_shader_program("res/vertex_shader.spv", "res/fragment_shader.spv");
        glUseProgram(shader_program);

        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if(grid_to_render == nullptr) continue;
            {
                std::lock_guard guard(mutex);
                if(grid_to_render == nullptr) continue;

                GLfloat triangles[grid_to_render->rows()][grid_to_render->cols()][2][3][3];
                const float grid_dx = 1.0 / static_cast<float>(grid_to_render->rows()),
                    grid_dy = 1.0 / static_cast<float>(grid_to_render->cols());
                for (int i = 1; i<grid_to_render->cols(); i++) {
                    for (int j = 1; j<grid_to_render->rows(); j++) {
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

                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, sizeof(triangles) / sizeof(triangles[0][0][0][0]));
                std::cout << "Drawing triangles with " << sizeof(triangles) / sizeof(triangles[0][0][0][0]) << " points" << std::endl;
                glfwSwapBuffers(window);

                grid_to_render = nullptr;
            }
        }
    }
    static void error_callback(int error, const char* description){
        std::cerr << "Error: " << description << '\n';
    }
    static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        std::cerr << type << "," << id << "," << severity << "," << std::string_view(message, length) << '\n';
    }
    void render(const Grid& grid) {
        std::lock_guard guard(mutex);
        grid_to_render = &grid; // TODO: maybe copy it so we don't render it while it's overwritten
    }
};

int main() {
    MyGLFW myGlfw;

    World world;

    for (int i = 0; i < 100; i++) {
        world.step();
        myGlfw.render(world.grid());
    }
}
