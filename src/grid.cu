#include "grid.hpp"
#include "scheme.hpp"
#include <cassert>
#include <stdexcept>

void* CUDAMalloc::calloc(std::size_t num, std::size_t size) {
    void* mem;
    auto success = cudaMallocManaged(&mem, num * size);
    if(success != cudaSuccess) {
        throw std::runtime_error(cudaGetErrorName(success));
    }
    memset(mem, 0, num * size);
    return mem;
}

void CUDAMalloc::free(void* mem) {
    cudaFree(mem);
}

void synchronize() {
#ifndef NO_CUDA
    cudaDeviceSynchronize();
#endif
}
