#ifndef VIEWER_H
#define VIEWER_H

#include "my_glfw.hpp"
#include <mutex>
#include <stdexcept>
#include <thread>

class Viewer {
public:
    struct WindowClosed : std::runtime_error {
        WindowClosed(): std::runtime_error("window closed") {}
    };
private:
    MyGLFW _glfw;
    std::thread ui_thread;
    std::mutex mutex;
    Grid grid_copy;
    const Grid* grid_to_render = nullptr; // Protected by the mutex
    bool closed = false;
public:
    Viewer() {
        ui_thread = std::thread([this](){ this->thread_main(); });
    }
    ~Viewer() {
        if(ui_thread.joinable()){
            {
                std::lock_guard guard(mutex);
                _glfw.signal_should_close();
            }
            ui_thread.join();
        }
    }
    void thread_main() {
        _glfw.initialize();
        while(!_glfw.closed()) {
            if(grid_to_render != nullptr) {
                std::lock_guard guard(mutex);
                if(grid_to_render != nullptr){ // Double checked locking
                    _glfw.set_grid(grid_to_render);
                }
            }
            _glfw.render();
        }
        closed = true;
    }
    void render(const Grid& grid) {
        std::lock_guard guard(mutex);
        if (closed) {
            throw WindowClosed();
        }
        grid_to_render = &grid; // TODO: maybe copy it so we don't render it while it's overwritten
    }
};

#endif //VIEWER_H
