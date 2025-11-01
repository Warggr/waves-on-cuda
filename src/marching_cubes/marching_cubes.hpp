#include "grid.hpp"
#include "generated/marching_cubes_cache.hpp"
#include <array>
#include <vector>

struct Point3D {
    double x, y, z;
    Point3D() = default;
    Point3D(std::array<double, 3> arr): x(arr[0]), y(arr[1]), z(arr[2]) {}
};

struct Triangle {
    std::array<Point3D, 3> corners;
};

void marching_cube(int x, int y, int z, double isoLevel, const Grid<double, 3>& grid, std::vector<Triangle>& out);

std::vector<Triangle> marching_cubes(const Grid<double, 3>& grid, double isoLevel);
