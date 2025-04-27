#include "grid.hpp"
#include "viewer.hpp"

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
