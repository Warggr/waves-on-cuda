#include "vof/vof.hpp"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <span>

namespace py = pybind11;

std::tuple<py::array_t<double, 3>, py::array_t<double, 3>>
get_wall_sizes_py(double volume_fraction, const py::array_t<double>& normals) {
    py::buffer_info buf1 = normals.request();
    if(buf1.size != 3)
        throw std::runtime_error("Input array size mismatch");

    std::span<double, 3> normals_cpp{static_cast<double*>(buf1.ptr),
                                     static_cast<std::size_t>(buf1.size)};
    const auto& [wall_sizes_early, wall_sizes_late] =
        get_wall_sizes(volume_fraction, normals_cpp);

    py::array_t<double, 3> wall_sizes_early_py(3, wall_sizes_early.data()),
        wall_sizes_late_py(3, wall_sizes_late.data());
    return std::make_tuple(wall_sizes_early_py, wall_sizes_late_py);
}

py::array_t<double, 12> get_intersect_py(double volume_fraction,
                                         const py::array_t<double>& normals) {
    py::buffer_info buf1 = normals.request();
    if(buf1.size != 3)
        throw std::runtime_error("Input array size mismatch");

    std::span<double, 3> normals_cpp{static_cast<double*>(buf1.ptr),
                                     static_cast<std::size_t>(buf1.size)};
    const auto intersect = get_intersect(volume_fraction, normals_cpp);

    return py::array_t<double, 12>(12, intersect.data());
}

PYBIND11_MODULE(vof_bindings, m) {
    m.def("get_wall_sizes", &get_wall_sizes_py);
    m.def("get_intersect", &get_intersect_py);
}
