#pragma once
#include <array>
#include <utility>

#include "ArrayView.hpp"

constexpr int GRID_WIDTH = 100,
    GRID_HEIGHT = 100;

struct Grid {
    double* _data;
    Grid();
    ~Grid();
    struct row {
        double* _data;
        row(double* data) : _data(data) {}
        row operator+() const { return {_data + GRID_WIDTH}; }
        ArrayView<double, GRID_WIDTH> operator*() const { return {_data}; }
    };
    struct const_row {
        const double* _data;
        const_row(const double* data) : _data(data) {}
        const_row operator+() const { return {_data + GRID_WIDTH}; }
        ArrayView<const double, GRID_WIDTH> operator*() const { return {_data}; }
    };

    row begin() const { return { _data }; }
    row end() const { return { _data + GRID_WIDTH * GRID_HEIGHT }; }
    const_row cbegin() const { return { _data }; }
    const_row cend() const { return { _data + GRID_WIDTH * GRID_HEIGHT }; }
    std::size_t size() const { return GRID_WIDTH * GRID_HEIGHT; }
    std::size_t cols() const { return GRID_WIDTH; }
    std::size_t rows() const { return GRID_HEIGHT; }
    double* operator[] (int i) { return _data + i*GRID_WIDTH; }
    const double* operator[] (int i) const { return _data + i*GRID_WIDTH; }
};

class World {
    Grid grid1, grid2;
    Grid* current_grid, * other_grid;
public:
    World() {
        current_grid = &grid1;
        other_grid = &grid2;
    }
    void step();
    const Grid& grid() const { return *current_grid; }
};
