#include "grid.hpp"
#include <stdexcept>
#include <cassert>

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

constexpr double SINE_FREQ = 2.0; // in 1 / time unit
constexpr double SINE_FREQ_2PI = SINE_FREQ * 2 * M_PI;
constexpr double WAVE_SPEED = 0.5; // in space unit / time unit.

#ifndef NO_CUDA
__global__
#endif
// C is the Courant number, c = v dx/dt. It must hold that 0 <= c < 1
void cuda_step(const double* in, PlainCGrid out, double t, double c, std::size_t grid_width, std::size_t grid_height) {
    for(int i = 0; i < grid_height; i++) {
        out[i*grid_width] = sin(t * SINE_FREQ_2PI) + 1;
        for(int j = 1; j<grid_width; j++) {
            out[i*grid_width + j] =  in[i*grid_width + j] - c * (in[i*grid_width + j] - in[i*grid_width + j-1]);
        }
    }
}

void World::step(bool sync) {
    const double c = WAVE_SPEED * grid1.cols() * dt;
    assert(c <= 1.0);
#ifndef NO_CUDA
    cuda_step<<< 1, 1 >>>(current_grid->_data, other_grid->_data, t, c, other_grid->rows(), other_grid->cols());
    if (sync) {
        cudaDeviceSynchronize();
    }
#else
    cuda_step(current_grid->_data, other_grid->_data, t, c, other_grid->rows(), other_grid->cols());
#endif
    std::swap(other_grid, current_grid);
    t += dt;
}
