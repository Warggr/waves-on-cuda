#include "grid.hpp"
#include "renderer.hpp"

int main(int argc, char** argv) {
	Grid<double, 3> grid({ 10, 10, 10 });
	for(const auto& [i, j, k]: grid.indices()){
		grid[i][j][k] = (i-5)*(i-5) + (j-5)*(j-5) + (k-5)*(k-5);
	}

	Renderer3D renderer;
	renderer.initialize();
	renderer.set_grid(grid);
	while(!renderer.closed()){
		renderer.render();
	}
}
