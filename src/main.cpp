#include "grid.hpp"
#include <iostream>

int main() {
    World world;

    for(int t = 0; t<50; t++) {
        world.step();
    }
    for(const auto& row: world.grid()) {
        for(const auto& cell: row) {
            if(cell > 0) std::cout << cell;
            else std::cout << "  ";
        }
        std::cout << std::endl;
    }
}
