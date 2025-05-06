#include "grid.hpp"
#include <stdexcept>

using PlainCGrid = double*;

Grid::Grid(std::size_t grid_height, std::size_t grid_width): _grid_height(grid_height), _grid_width(grid_width) {
    auto success = cudaMallocManaged(&_data, _grid_width * _grid_height * sizeof(double));
    if (success != cudaSuccess) {
        throw std::runtime_error(cudaGetErrorName(success));
    }
    for (int i = 0; i < _grid_width * _grid_height; i++) {
        _data[i] = 0;
    }
}

Grid::~Grid() {
    cudaFree(_data);
}

#ifndef NO_CUDA
__global__
#endif
void cuda_step(const double* in, PlainCGrid out, std::size_t grid_width, std::size_t grid_height) {
    for(int i = 0; i < grid_height; i++) {
        out[i*GRID_WIDTH] = 1.0;
        for(int j = 1; j<grid_width; j++) {
            out[i*GRID_WIDTH + j] = in[i*GRID_WIDTH + j-1];
        }
    }
}

void World::step() {
#ifndef NO_CUDA
    cuda_step<<< 1, 1 >>>(current_grid->_data, other_grid->_data, other_grid->rows(), other_grid->cols());
#else
    cuda_step(current_grid->_data, other_grid->_data, other_grid->rows(), other_grid->cols());
#endif
    std::swap(other_grid, current_grid);
}
