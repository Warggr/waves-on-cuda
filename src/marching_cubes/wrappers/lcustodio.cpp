#include "MarchingCubes.h"
#include "marching_cubes.hpp"
#include "grid.hpp"
#ifdef TIMING
#include "timing.hpp"
#endif

namespace waves_on_cuda::marching_cubes {

static geometry::Point3D<float> convert(const ::Vertex& v) {
    return {{ v.x, v.y, v.z }};
}

std::vector<geometry::Triangle<float>> marching_cubes(const Grid<double, 3> &grid, double isoLevel) {
    MarchingCubes mc;
    auto shape = grid.shape();
    mc.set_resolution(shape[0], shape[1], shape[2]);
    mc.init_all();
    for(const auto [i, j, k]: grid.indices()){
        mc.set_data(grid[i][j][k] - isoLevel, i, j, k);
    }
    {
#ifdef TIMING
        ScopeTimer s;
	    for(int i = 0; i < 1000; i++)
#endif
        {
            mc.run();
        }
    }
    mc.clean_temps();

    std::vector<geometry::Triangle<float>> result;
    ::Triangle* triangles = mc.triangles();
    ::Vertex* vertices = mc.vertices();
    float dx = 1.0 / (grid.shape()[0] - 1);
    float dy = 1.0 / (grid.shape()[1] - 1);
    float dz = 1.0 / (grid.shape()[2] - 1);
    for(int i = 0; i < mc.nverts(); i++){
        Vertex &v = vertices[i];
        v.x *= dx; v.y *= dy; v.z *= dz;
    }

    for(int i = 0; i < mc.ntrigs(); i++){
        const ::Triangle& tr = triangles[i];
        result.push_back(geometry::Triangle<float>({
            convert(vertices[tr.v1]),
            convert(vertices[tr.v2]),
            convert(vertices[tr.v3])
        }));
    }
    return result;
}

}
