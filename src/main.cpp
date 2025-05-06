#include "grid.hpp"
#include "viewer.hpp"
#include <ostream>
#include <chrono>

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
    for (const auto& row : grid) {
        for (const auto& col : row) {
            os << col << " ";
        }
        os << "\n";
    }
    return os;
}

int main() {
    using namespace std::chrono;

    const double dt = 0.01;
    const std::size_t GRID_WIDTH = 100, GRID_HEIGHT = 100;
    const steady_clock::duration dt_as_duration = duration_cast<steady_clock::duration>(duration<float, std::milli>(1000 * dt));

    World world(GRID_HEIGHT, GRID_WIDTH, dt);

    Viewer myGlfw;

    auto tick_time = steady_clock::now();

    try {
        while (true) {
            world.step();
            myGlfw.render(world.grid());
            tick_time += dt_as_duration;
            std::this_thread::sleep_until(tick_time);
        }
    } catch (const Viewer::WindowClosed&) {

    }
}
