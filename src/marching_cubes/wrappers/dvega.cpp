#include "marching_cubes.hpp"
#include "grid.hpp"

#define GRD_data_type double
#define _ORTHO_GRD
#include "marching_cubes_33.h"
#include "timing.hpp"
#include <cassert>

int* getTriangle(surface* S, int n) { return S->T[n>>_MC_N][n&_MC_A]; }

float* getVertex(surface* S, int n) { return S->V[n>>_MC_N][n&_MC_A]; }

float* getNormal(surface* S, int n) { return S->N[n>>_MC_N][n&_MC_A]; }

struct GRD_wrapper {
	_GRD lib_grid;
	std::vector<const GRD_data_type*> dim1;
	std::vector<const GRD_data_type**> dim2;
};

#define XSTR(x) #x
#define STR(x) XSTR(x)

GRD_wrapper native_to_lib(const Grid<double, 3>& grid) {
	GRD_wrapper result;
	_GRD& lib_grid = result.lib_grid;
	for(int j = 0; j < 3; j++){
		// They count the number of intervals, we count the number of points
		lib_grid.N[j] = grid.shape()[j] - 1;
		lib_grid.L[j] = 1.0;
		lib_grid.d[j] = 1.0 / (grid.shape()[j] - 1);
		lib_grid.r0[j] = 0.0;
	}
	// The const_cast is a workaround for the lib, which does not modify
	// its argument but still doesn't declare it as const
	result.dim2.resize(grid.size() / grid[0][0].size());
	for(const auto& sheet: grid) {
		for(const auto& line: sheet) {
			result.dim1.push_back(line.data());
		}
	}
	unsigned sheet_size_in_lines = grid[0].size() / grid[0][0].size();
	result.dim2.resize(grid.shape()[0]);
	for(int i = 0; i < grid.shape()[0]; i++){
		result.dim2[i] = &result.dim1[i*sheet_size_in_lines];
	}
	lib_grid.F = const_cast<GRD_data_type***>(&(*result.dim2.begin()));
	for(const auto& [i, j, k]: grid.indices()){
		assert(lib_grid.F[i][j][k] == grid[i][j][k]);
		//printf("%f @ [%lu][%lu][%lu] = %p\n", grid[i][j][k], i, j, k, &grid[i][j][k]);
	}

	return result;
}

namespace waves_on_cuda::marching_cubes {

using geometry::Triangle;
std::vector<Triangle<float>> marching_cubes(const Grid<double, 3>& grid, double isoLevel){
	GRD_wrapper Z = native_to_lib(grid);

	surface* S;
	{
	#ifdef TIMING
		ScopeTimer s;
		for(int i = 0; i < 1000; i++)
	#endif
		{
			S = calculate_isosurface(&Z.lib_grid, isoLevel);
		}
	}
	std::vector<Triangle<float>> result;

	// This is also a workaround for the lib, which sets this to -1 if there are no triangles
	if(S->nT == -1)
		return result;

	for (int n = S->nT; n != 0; n--)
	{
		int* t = getTriangle(S,n);
		Triangle<float> tri;

		for(int j=0;j<3;j++)
		{
			float* vertex = getVertex(S,t[j]);
			tri.corners[j].x = vertex[0];
			tri.corners[j].y = vertex[1];
			tri.corners[j].z = vertex[2];
			// getNormal(S,t[j]);
		}
		result.push_back(tri);
	}
	free_surface_memory(&S);

	return result;
}

}
