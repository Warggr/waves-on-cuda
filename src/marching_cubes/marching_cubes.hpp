#include "grid.hpp"
#include <array>
#include <vector>

#pragma once

namespace waves_on_cuda::marching_cubes {

namespace geometry {

template<typename dtype>
struct Point3D {
    dtype x, y, z;
    Point3D() = default;
    Point3D(std::array<dtype, 3> arr): x(arr[0]), y(arr[1]), z(arr[2]) {}
};

template<typename dtype>
struct Triangle {
    std::array<Point3D<dtype>, 3> corners;
    Triangle(){};
    Triangle(const std::array<Point3D<dtype>,3>& corners)
    : corners(corners)
    {}
};

}

std::vector<geometry::Triangle<float>> marching_cubes(const Grid<double, 3>& grid, double isoLevel);

}
