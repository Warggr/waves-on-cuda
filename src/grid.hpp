#pragma once

#include "ArrayView.hpp"

template<class T>
struct SpanView {
    T* _data;
    std::size_t _size;

    T* begin() const { return _data; }
    T* end() const { return _data + _size; }
};

struct Grid {
    std::size_t _grid_height, _grid_width;
    double* _data;
    Grid(std::size_t grid_height, std::size_t grid_width);
    ~Grid();

    void reset();

    template<class T>
    struct row {
        T* _data;
        std::size_t _row_width, _nb_rows;

        T* begin() const { return _data; }
        T* end() const { return _data + _row_width * _nb_rows; }
        void operator++() { _data += _row_width; }
        bool operator!=(const row& other) const { return _data != other._data; }
        bool operator!=(const T* other) const { return _data != other; }
        SpanView<T> operator*() const { return {_data, _row_width}; }
    };

    row<double> begin() const { return { _data, _grid_width, _grid_height }; }
    row<const double> cbegin() const { return { _data, _grid_width }; }
    const double* end() const { return { _data + _grid_width * _grid_height }; }
    std::size_t size() const { return _grid_width * _grid_height; }
    std::size_t cols() const { return _grid_width; }
    std::size_t rows() const { return _grid_height; }
    double* operator[] (int i) { return _data + i*_grid_width; }
    const double* operator[] (int i) const { return _data + i*_grid_width; }
};

class World {
    Grid grid1, grid2;
    Grid* current_grid, * other_grid;
    double t = 0;
    double dt;
public:
    World(std::size_t grid_height, std::size_t grid_width, double dt = 0.01):
        dt(dt), grid1(grid_height, grid_width), grid2(grid_height, grid_width)
    {
        current_grid = &grid1;
        other_grid = &grid2;
    }
    void step(bool sync = false);
    const Grid& grid() const { return *current_grid; }
    inline void reset() { current_grid->reset(); }
    void synchronize();
};
