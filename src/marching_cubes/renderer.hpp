#include "grid.hpp"
#include "marching_cubes.hpp"
#include "my_glfw.hpp"

using namespace waves_on_cuda::marching_cubes;
using Triangle = geometry::Triangle<float>;

class Renderer3D: public MyGLFW {
    float isoLevel;
public:
    Renderer3D(float isoLevel): isoLevel(isoLevel) {
    }
    void set_grid(const Grid<double, 3>& grid) {
        std::vector<Triangle> triangles = marching_cubes(grid, isoLevel);
        std::vector<GLfloat> vertex_data;
        vertex_data.reserve(triangles.size() * 9);

        for (const auto& tri : triangles) {
            for (const auto& p : tri.corners) {
                vertex_data.push_back(p.x);
                vertex_data.push_back(p.y);
                vertex_data.push_back(p.z);
            }
        }

        set_triangles(vertex_data);
    }
};
