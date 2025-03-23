#pragma once
#include <array>
#include <utility>

constexpr int GRID_WIDTH = 100,
    GRID_HEIGHT = 100;

struct Grid {
    using GridArray = std::array<std::array<double, GRID_WIDTH>, GRID_HEIGHT>;
    GridArray _data;
    Grid() {
        for(auto& row: _data) {
            for(double& cell: row) {
                cell = 0;
            }
        }
    }
    GridArray::iterator begin() { return _data.begin(); }
    GridArray::iterator end() { return _data.end(); }
    GridArray::const_iterator begin() const { return _data.begin(); }
    GridArray::const_iterator end() const { return _data.end(); }
    std::size_t size() const { return _data.size(); }
    std::size_t cols() const { return GRID_WIDTH; }
    std::size_t rows() const { return GRID_HEIGHT; }
    std::array<double, GRID_WIDTH>& operator[] (int i) { return _data[i]; }
    const std::array<double, GRID_WIDTH>& operator[] (int i) const { return _data[i]; }
};

class World {
    Grid grid1, grid2;
    Grid* current_grid, * other_grid;
public:
    World() {
        current_grid = &grid1;
        other_grid = &grid2;
    }
    void step() {
        for(int i = 0; i < GRID_HEIGHT; i++) {
            (*other_grid)[i][0] = 1.0;
            for(int j = 1; j<GRID_WIDTH; j++) {
                (*other_grid)[i][j] = (*current_grid)[i][j-1];
            }
        }
        std::swap(other_grid, current_grid);
    }
    const Grid& grid() const { return *current_grid; }
};
