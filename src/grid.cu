#include "grid.hpp"
#include "scheme.hpp"
#include <stdexcept>
#include <cassert>

template class Grid<double, 2>;
template class Grid<double, 3>;

using PlainCGrid = double*;

void* CUDAAllocator::calloc(std::size_t num, std::size_t size) {
    void* mem;
    auto success = cudaMallocManaged(&mem, num * size);
    if (success != cudaSuccess) {
        throw std::runtime_error(cudaGetErrorName(success));
    }
    memset(mem, 0, size);
    return mem;
}

void CUDAAllocator::free(void* mem) {
    cudaFree(mem);
}

void synchronize() {
#ifndef NO_CUDA
    cudaDeviceSynchronize();
#endif
}
