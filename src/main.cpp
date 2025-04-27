#include "grid.hpp"
#include "viewer.hpp"
#include <ostream>

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
    World world;

    Viewer myGlfw;

    try {
        while (true) {
            world.step();
            myGlfw.render(world.grid());
        }
    } catch (const Viewer::WindowClosed&) {

    }
}
