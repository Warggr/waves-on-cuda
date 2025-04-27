#pragma once

#include "ArrayView.hpp"

constexpr int GRID_WIDTH = 100,
    GRID_HEIGHT = 100;

struct Grid {
    double* _data;
    Grid();
    ~Grid();

    template<class dtype>
    struct row {
        dtype* _data;
        row(dtype* data) : _data(data) {}
        void operator++() { _data += GRID_WIDTH; }
        bool operator!=(const row& other) const { return _data != other._data; }
        ArrayView<dtype, GRID_WIDTH> operator*() const { return {_data}; }
    };

    row<double> begin() const { return { _data }; }
    row<double> end() const { return { _data + GRID_WIDTH * GRID_HEIGHT }; }
    row<const double> cbegin() const { return { _data }; }
    row<const double> cend() const { return { _data + GRID_WIDTH * GRID_HEIGHT }; }
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
