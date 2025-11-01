#include "marching_cubes.hpp"
#include "grid.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
	std::ifstream cache;
	cache.open("lookup_tables.dat", std::ios::binary);
	if(!cache.is_open()) {
		std::cerr << "Couldn't open cache `lookup_tables.dat'!" << std::endl;
		return 1;
	}

	Grid<double, 3> grid({ 10, 10, 10 });
}
