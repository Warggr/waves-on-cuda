#include "grid.hpp"
#include <stdexcept>
#include <cassert>

template class Grid<double, 2>;

using PlainCGrid = double*;

template<class dtype, unsigned int dim>
Grid<dtype, dim>::Grid(std::array<const std::size_t, dim>&& dimensions):
    _size(std::move(dimensions)),
    GridView<dtype, dim>(nullptr, this->_size)
{
    auto success = cudaMallocManaged(&this->_data, this->size() * sizeof(dtype));
    if (success != cudaSuccess) {
        throw std::runtime_error(cudaGetErrorName(success));
    }
    reset();
}

template<class dtype, unsigned int dim>
Grid<dtype, dim>::~Grid() {
    cudaFree(this->_data);
}

void World::synchronize() {
#ifndef NO_CUDA
    cudaDeviceSynchronize();
#endif
}

template<class dtype, unsigned int dim>
void Grid<dtype, dim>::reset() {
    memset(this->_data, 0, this->size() * sizeof(dtype));
}

constexpr double SINE_FREQ = 2.0; // in 1 / time unit
constexpr double SINE_FREQ_2PI = SINE_FREQ * 2 * M_PI;
constexpr double WAVE_SPEED = 0.5; // in space unit / time unit.

void nocuda_step(
    const double* in,
    PlainCGrid out,
    double t,
    double c, // C is the Courant number, c = v dx/dt. It must hold that 0 <= c < 1
    std::size_t grid_width, std::size_t grid_height
) {
    for(int i = 0; i < grid_height; i++) {
        out[i*grid_width] = sin(t * SINE_FREQ_2PI) + 1;
        for(int j = 1; j<grid_width; j++) {
            out[i*grid_width + j] =  in[i*grid_width + j] - c * (in[i*grid_width + j] - in[i*grid_width + j-1]);
        }
    }
}

__device__
inline void _cuda_step(
    const double* in,
    PlainCGrid out,
    double t,
    double c, // C is the Courant number, c = v dx/dt. It must hold that 0 <= c < 1
    std::size_t grid_width, std::size_t grid_height,
    std::size_t block_size
) {
    int start = threadIdx.x * block_size;
    int end = (threadIdx.x + 1) * block_size;
    for(int i = start; i < end; i++) {
        out[i*grid_width] = sin(t * SINE_FREQ_2PI) + 1;
        for(int j = 1; j<grid_width; j++) {
            out[i*grid_width + j] =  in[i*grid_width + j] - c * (in[i*grid_width + j] - in[i*grid_width + j-1]);
        }
    }
}

__global__ void cuda_step(PlainCGrid in, PlainCGrid out, double t, double c, std::size_t grid_width, std::size_t grid_height, std::size_t block_size) {
    _cuda_step(in, out, t, c, grid_width, grid_height, block_size);
}

__global__
void cuda_multistep(
    PlainCGrid in,
    PlainCGrid out,
    double t,
    double c,
    std::size_t grid_width, std::size_t grid_height,
    std::size_t block_size,
    unsigned N
) {
    for(unsigned i = 0; i < N; i++){
        _cuda_step(in, out, t, c, grid_width, grid_height, block_size);
	// can't use std::swap in device code, have to code our own
	auto tmp = in; in = out; out = tmp;
    }
}

void World::step() {
    const double c = WAVE_SPEED * grid1.shape()[0] * dt;
    assert(c <= 1.0);
#ifndef NO_CUDA
    cuda_step<<< 1, other_grid->shape()[1] >>>(current_grid->data(), other_grid->data(), t, c, other_grid->shape()[1], other_grid->shape()[0], 1);
#else
    nocuda_step(current_grid->_data, other_grid->_data, t, c, other_grid->rows(), other_grid->cols());
#endif
    std::swap(other_grid, current_grid);
    t += dt;
}

void World::multi_step(unsigned N) {
    const double c = WAVE_SPEED * grid1.shape()[0] * dt;
    assert(c <= 1.0);
#ifndef NO_CUDA
    cuda_multistep<<< 1, other_grid->shape()[1] >>>(current_grid->data(), other_grid->data(), t, c, other_grid->shape()[1], other_grid->shape()[0], 1, N);
#else
    for(unsigned i = 0; i < N; i++){
        nocuda_step(current_grid->_data, other_grid->_data, t, c, other_grid->rows(), other_grid->cols());
        std::swap(other_grid, current_grid);
    }
#endif
    t += N * dt;
}
