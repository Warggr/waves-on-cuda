#include "marching_cubes.hpp"
#include "grid.hpp"
#include "generated/marching_cubes_cache.hpp"
#ifdef TIMING
#include "timing.hpp"
#endif
#include <span>

using std::size_t;

template<typename d, typename idx_t, std::size_t size>
void permute(std::span<d, size> original,
             const std::span<idx_t, size> permutation) {
    std::array<d, size> result;
    for(size_t i = 0; i < size; i++) {
        result[permutation[i]] = original[i];
    }
    for(size_t i = 0; i < size; i++) {
        original[i] = result[i];
    }
}

namespace waves_on_cuda::marching_cubes {

using geometry::Point3D;
using geometry::Triangle;

// Interior test between vertices 1 and 7.
bool interior_test(const std::array<float, NB_VERTICES>& v) {
    const auto [a0, d0, b0, c0, a1, d1, b1, c1] = v;
    float a = (a1 - a0) * (c1 - c0) - (b1 - b0) * (d1 - d0);
    float b = c0 * (a1 - a0) + a0 * (c1 - c0) - d0 * (b1 - b0) - b0 * (d1 - d0);
    // float c = a0*c0 - b0*d0;
    if(a >= 0) {
        float t = -0.5 * b / a;
        if(0 <= t && t <= 1) {
            float At = a0 + t * (a1 - a0), Bt = b0 + t * (b1 - b0),
                  Ct = c0 + t * (c1 - c0), Dt = d0 + t * (d1 - d0);
            if((Ct > 0) == (At > 0) && At * Ct - Bt * Dt > 0) {
                return ((At > 0) == (a0 > 0)) || ((Ct > 0) == (c0 > 0));
            }
        }
    }
    return false;
}

void marching_cube(int x, int y, int z, double isoLevel,
                   const GridView<double, 3>& grid,
                   std::vector<Triangle<float>>& out) {
    // Fetch 8 corner values
    std::array<float, NB_VERTICES> v;
    for(int i = 0; i < NB_VERTICES; i++) {
        v[i] = grid[z + ((i >> 2) & 1)][y + ((i >> 1) & 1)][x + (i & 1)];
    }

    std::array<Point3D<float>, NB_EDGES + 1> intersect_and_center;
    std::span<Point3D<float>, NB_EDGES> intersect =
        std::span(intersect_and_center).first<NB_EDGES>();
    std::array<float, 3> base = {static_cast<float>(z), static_cast<float>(y),
                                 static_cast<float>(x)};
    intersect_and_center[NB_EDGES] =
        Point3D<float>({base[0] + 0.5f, base[1] + 0.5f, base[2] + 0.5f});
    for(int i = 0; i < NB_EDGES; i++) {
        auto edge = cube_geometry.edge_definition[i];
        double a = v[edge.a], b = v[edge.b];
        std::array<float, 3> midpoint = {static_cast<float>(edge.x),
                                         static_cast<float>(edge.y),
                                         static_cast<float>(edge.z)};
        midpoint[edge.changing_dim] = (a - isoLevel) / (a - b);
        std::array<float, 3> midpoint_scale;
        for(int j = 0; j < 3; j++)
            midpoint_scale[j] = (base[j] + midpoint[j]) / (grid.shape()[j] - 1);
        intersect[i] = Point3D(midpoint_scale);
    }

    unsigned char index_ = 0;
    for(int i = 0; i < 8; i++) {
        if(v[i] > isoLevel)
            index_ += (1 << i);
    }
    const auto& case_ptr = lookup_table.case_table[index_];
    permute(intersect,
            std::span(cube_geometry.all_permutations[case_ptr.permutation]
                          .edge_permutation));
    permute(std::span(v),
            std::span(cube_geometry.all_permutations[case_ptr.permutation]
                          .vertex_permutation));
    bool sign_flip = case_ptr.sign_flip;

    const Case& _case = lookup_table.all_cases[case_ptr._case];

    int test = 0;
    for(int i = 0; i < _case.num_tests; i++) {
        int side = _case.tests[i];
        if(side == 6) {
            test += (interior_test(v) ? (1 << i) : 0);
        } else {
            float a = v[cube_geometry.adjacency[side][0]],
                  b = v[cube_geometry.adjacency[side][1]],
                  c = v[cube_geometry.adjacency[side][2]],
                  d = v[cube_geometry.adjacency[side][3]];
            test += ((a * c - b * d) > isoLevel) ? (1 << i) : 0;
        }
    }
    const auto& subcase_ptr = _case.subcases[test];
    sign_flip = sign_flip ^ subcase_ptr.sign_flip;
    const auto& subcase = lookup_table.all_subcases[subcase_ptr.subcase];
    permute(intersect,
            std::span(cube_geometry.all_permutations[subcase_ptr.permutation]
                          .edge_permutation));

    for(int i = 0; i < subcase.num_triangles; i++) {
        Triangle<float> tri;
        for(size_t j = 0; j < 3; j++) {
            if(sign_flip) {
                tri.corners[2 - j] =
                    intersect_and_center[subcase.triangles[i][j]];
            } else {
                tri.corners[j] = intersect_and_center[subcase.triangles[i][j]];
            }
        }
        out.push_back(tri);
    }
}

std::vector<Triangle<float>> marching_cubes(const GridView<double, 3>& grid,
                                            double isoLevel) {
    std::vector<Triangle<float>> out;
    {
#ifdef TIMING
        ScopeTimer s;
        for(int i = 0; i < 1000; i++)
#endif
        {
            for(size_t i = 0; i < grid.shape()[0] - 1; i++) {
                for(size_t j = 0; j < grid.shape()[1] - 1; j++) {
                    for(size_t k = 0; k < grid.shape()[2] - 1; k++) {
                        marching_cube(i, j, k, isoLevel, grid, out);
                    }
                }
            }
        }
    }
    return out;
}

}
