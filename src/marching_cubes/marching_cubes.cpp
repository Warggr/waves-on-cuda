#include "marching_cubes.hpp"
#include "grid.hpp"
#include "generated/marching_cubes_cache.hpp"
#include "timing.hpp"

using std::size_t;

template<typename d, typename idx_t, std::size_t size>
std::array<d, size> permute(std::array<d, size> original, std::array<idx_t, size> permutation) {
    std::array<d, size> result;
    for(size_t i = 0; i < size; i++){
        result[permutation[i]] = original[i];
    }
    return result;
}

namespace waves_on_cuda::marching_cubes {

using geometry::Triangle;
using geometry::Point3D;

void marching_cube(int x, int y, int z, double isoLevel, const Grid<double, 3>& grid, std::vector<Triangle<float>>& out){
    // Fetch 8 corner values
    std::array<float, NB_VERTICES> v;
    for(int i = 0; i < NB_VERTICES; i++){
        v[i] = grid[z + ((i>>2)&1)][y + ((i>>1)&1)][x + (i&1)];
    }

    std::array<Point3D<float>, NB_EDGES> intersect;
    std::array<float, 3> base = { static_cast<float>(z), static_cast<float>(y), static_cast<float>(x) };
    for(int i = 0; i < NB_EDGES; i++){
        auto edge = cube_geometry.edge_definition[i];
        double a = v[edge.a],
               b = v[edge.b]; 
        std::array<float, 3> midpoint = { static_cast<float>(edge.x), static_cast<float>(edge.y), static_cast<float>(edge.z) };
        midpoint[edge.changing_dim] = (a - isoLevel) / (a - b);
        std::array<float, 3> midpoint_scale;
        for(int j = 0; j < 3; j++) midpoint_scale[j] = (base[j] + midpoint[j]) / (grid.shape()[j] - 1);
        intersect[i] = Point3D(midpoint_scale);
    }

    unsigned char index_ = 0;
    for(int i = 0; i < 8; i++){
        if(v[i] > isoLevel) index_ += (1 << i);
    }
    const auto& case_ptr = lookup_table.case_table[index_];
    intersect = permute(intersect, cube_geometry.all_permutations[case_ptr.permutation].edge_permutation);
    v = permute(v, cube_geometry.all_permutations[case_ptr.permutation].vertex_permutation);
    bool sign_flip = case_ptr.sign_flip;

    const Case& _case = lookup_table.all_cases[case_ptr._case];

    int test = 0;
    for(int i = 0; i<_case.num_tests; i++){
        int side = _case.tests[i];
        if(side == 6){
            test += (true ? 1 : 0); // TODO
        } else {
            float a = v[cube_geometry.adjacency[side][0]],
                  b = v[cube_geometry.adjacency[side][1]],
                  c = v[cube_geometry.adjacency[side][2]],
                  d = v[cube_geometry.adjacency[side][3]];
            test += ((a*c - b*d) > isoLevel) ? (1 << i) : 0;
        }
    }
    const auto& subcase_ptr = _case.subcases[test];
    sign_flip = sign_flip ^ subcase_ptr.sign_flip;
    const auto& subcase = lookup_table.all_subcases[subcase_ptr.subcase];
    intersect = permute(intersect, cube_geometry.all_permutations[subcase_ptr.permutation].edge_permutation);

    for(int i = 0; i < subcase.num_triangles; i++){
        Triangle<float> tri;
        for(size_t j = 0; j < 3; j++){
            if(sign_flip){
                tri.corners[2-j] = intersect[subcase.triangles[i][j]];
            } else {
                tri.corners[j] = intersect[subcase.triangles[i][j]];
            }
        }
        out.push_back(tri);
    }
}

std::vector<Triangle<float>> marching_cubes(const Grid<double, 3>& grid, double isoLevel) {
    std::vector<Triangle<float>> out;
    {
#ifdef TIMING
        ScopeTimer s;
        for(int i = 0; i < 1000; i++)
#endif
        {
            for(size_t i = 0; i < grid.shape()[0] - 1; i++){
                for(size_t j = 0; j < grid.shape()[1] - 1; j++){
                    for(size_t k = 0; k < grid.shape()[2] - 1; k++){
                        marching_cube(i, j, k, isoLevel, grid, out);
                    }
                }
            }
        }
    }
    return out;
}

}