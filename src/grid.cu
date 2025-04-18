#include "grid.hpp"

using PlainCGrid = double*;

Grid::Grid() {
    cudaMallocManaged(&_data, GRID_WIDTH * GRID_HEIGHT * sizeof(double));
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        _data[i] = 0;
    }
}

Grid::~Grid() {
    cudaFree(_data);
}

__global__
void cuda_step(PlainCGrid in, PlainCGrid out) {
    for(int i = 0; i < GRID_HEIGHT; i++) {
        out[i*GRID_WIDTH] = 1.0;
        for(int j = 1; j<GRID_WIDTH; j++) {
            in[i*GRID_WIDTH + j] = out[i*GRID_WIDTH + j-1];
        }
    }
}

void World::step() {
    cuda_step<<< 1, 1 >>>(grid1._data, grid2._data );
    std::swap(other_grid, current_grid);
}
