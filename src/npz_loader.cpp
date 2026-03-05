#include "npz_loader.hpp"
#include <cnpy/cnpy.hpp>
#include <stdexcept>

template<typename dtype, std::size_t ndim>
Grid<dtype, ndim> load(std::istream& infile) {
    cnpy::npy_array arr = cnpy::npy_load(infile);
    std::vector<std::size_t> loaded_shape = arr.shape();
    if(loaded_shape.size() != ndim) {
        throw std::runtime_error("Array shape mismatch");
    }
    std::array<std::size_t, ndim> shape;
    std::copy(loaded_shape.begin(), loaded_shape.end(), shape.begin());
    Grid<dtype, ndim> grid(shape);
    if(arr.word_size() != sizeof(dtype)) {
        throw std::runtime_error("dtype mismatch!");
    }
    if(arr.fortran_order()) {
        throw std::runtime_error("FORTRAN order not supported!");
    }
    std::copy(arr.data<dtype>(), arr.data<dtype>() + arr.num_vals(),
              grid.data());
    return grid;
}

template Grid<double, 3> load(std::istream& infile);
