#include "grid.hpp"

using PlainCGrid = double*;

void cuda_step(PlainCGrid in, PlainCGrid out) {
    for(int i = 0; i < GRID_HEIGHT; i++) {
        out[i*GRID_WIDTH] = 1.0;
        for(int j = 1; j<GRID_WIDTH; j++) {
            in[i*GRID_WIDTH + j] = out[i*GRID_WIDTH + j-1];
        }
    }
}

void World::step() {
    cuda_step(
        reinterpret_cast<double*>(current_grid->_data.data()),
        reinterpret_cast<double*>(other_grid->_data.data())
    );
    std::swap(other_grid, current_grid);
}
