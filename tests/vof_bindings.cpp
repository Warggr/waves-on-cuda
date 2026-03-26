#include "vof/intersect.hpp"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <span>

namespace py = pybind11;

std::tuple<py::array_t<double>, py::array_t<double>>
get_wall_sizes_py(double volume_fraction, const py::array_t<double>& normals) {
    py::buffer_info buf1 = normals.request();
    if(buf1.size != 3)
        throw std::runtime_error("Input array size mismatch");

    std::span<double, 3> normals_cpp{static_cast<double*>(buf1.ptr),
                                     static_cast<std::size_t>(buf1.size)};
    const auto& [wall_sizes_early, wall_sizes_late] =
        get_wall_sizes(volume_fraction, normals_cpp);

    py::array_t<double> wall_sizes_early_py(3, wall_sizes_early.data()),
        wall_sizes_late_py(3, wall_sizes_late.data());
    return std::make_tuple(wall_sizes_early_py, wall_sizes_late_py);
}

template<typename dtype, std::size_t dim1>
py::array_t<dtype> to_np(const std::array<dtype, dim1>& array) {
    using arr = py::array_t<dtype>;
    return arr{typename arr::ShapeContainer{dim1},
               typename arr::StridesContainer{sizeof(dtype)}, &array[0]};
}

template<typename dtype, std::size_t dim1, std::size_t dim2>
py::array_t<dtype>
to_np(const std::array<std::array<dtype, dim2>, dim1>& array) {
    using arr = py::array_t<dtype>;
    return arr(
        typename arr::ShapeContainer{dim1, dim2},
        typename arr::StridesContainer{dim2 * sizeof(dtype), sizeof(dtype)},
        &array[0][0]);
}

std::tuple<py::array_t<double>, py::array_t<bool>>
get_intersect_py(double volume_fraction, const py::array_t<double>& normals) {
    py::buffer_info buf1 = normals.request();
    if(buf1.size != 3)
        throw std::runtime_error("Input array size mismatch");

    std::span<double, 3> normals_cpp{static_cast<double*>(buf1.ptr),
                                     static_cast<std::size_t>(buf1.size)};
    const auto& [intersect, switched_sign] =
        get_intersect(volume_fraction, normals_cpp);

    return {to_np(intersect), to_np(switched_sign)};
}

PYBIND11_MODULE(vof_bindings, m) {
    py::array_t<double> baselines_py = to_np(baselines);
    py::array_t<unsigned int> switching_dim_py = to_np(switching_dim);

    m.def("get_wall_sizes", &get_wall_sizes_py);
    m.def("get_intersect", &get_intersect_py);
    m.attr("baselines") = baselines_py;
    m.attr("switching_dim") = switching_dim_py;
}
