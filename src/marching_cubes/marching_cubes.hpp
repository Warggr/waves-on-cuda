#include "grid.hpp"
#include <array>
#include <vector>

template<typename dtype>
struct Point3D {
    dtype x, y, z;
    Point3D() = default;
    Point3D(std::array<dtype, 3> arr): x(arr[0]), y(arr[1]), z(arr[2]) {}
};

template<typename dtype>
struct Triangle {
    std::array<Point3D<dtype>, 3> corners;
};

void marching_cube(int x, int y, int z, double isoLevel, const Grid<double, 3>& grid, std::vector<Triangle<float>>& out);

std::vector<Triangle<float>> marching_cubes(const Grid<double, 3>& grid, double isoLevel);
