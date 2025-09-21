#pragma once

#include "ArrayView.hpp"
#include <functional>
#include <numeric>

template<class dtype, unsigned int dimension>
class GridViewIterator;

template<class dtype, unsigned int dimension>
class GridView {
protected:
    ArrayView<const size_t, dimension> _grid_size_view;
    dtype* _data;
    GridView(dtype* data, const ArrayView<const size_t, dimension>& grid_size): _data(data), _grid_size_view(grid_size) {};
public:
    GridView(dtype* data, const std::array<const size_t, dimension>& grid_size):
	GridView(data, ArrayView<const size_t, dimension>{ grid_size.data() }) {}

    std::size_t size() const {
        return std::reduce(
            _grid_size_view.begin(), _grid_size_view.end(),
            1,
            std::multiplies<dtype>{}
        );
    }
    ArrayView<const size_t, dimension> shape() const { return _grid_size_view; }

    dtype* data() { return _data; }
    const dtype* data() const { return _data; }

    GridViewIterator<dtype, dimension-1> begin() const;
    const double* end() const { return { _data + size() }; }

    // Return a copy, because this is a View anyway (no data gets copied)
    GridView<dtype, dimension-1> operator[] (int i) const {
        return { _data + i*_grid_size_view[0], { _grid_size_view._data + 1 } };
    }

    friend class GridView<dtype, dimension+1>;
};

template<class dtype>
class GridView<dtype, 1>{
protected:
    dtype* _data;
    std::size_t _size;
    GridView(dtype* data, ArrayView<const size_t, 1> grid_size): _data(data), _size(grid_size[0]) {};
public:
    dtype* begin() const { return _data; }
    dtype* end() const { return _data + _size; }
    std::size_t size() const { return _size; }
    const dtype& operator[] (int i) const {
	return _data[i];
    }

    friend class GridView<dtype, 2>;
};

template<class dtype, unsigned int dimension>
struct GridViewIterator: public GridView<dtype, dimension> {
    GridViewIterator(dtype* data, ArrayView<const size_t, dimension> grid_size): GridView<dtype, dimension>(data, grid_size) {};
    void operator++() { this->_data += this->size(); }
    bool operator!=(const GridViewIterator& other) const { return this->_data != other._data; }
    // to compare with end()
    bool operator!=(const dtype* other) const { return this->_data != other; }
    GridView<dtype, dimension> operator*() const { return *this; }
};

template<class dtype, unsigned int dimension>
GridViewIterator<dtype, dimension-1> GridView<dtype, dimension>::begin() const {
    return {_data, _grid_size_view._data + 1 };
}

template<class dtype, unsigned int dimension>
class Grid: public GridView<dtype, dimension> {
    std::array<const size_t, dimension> _size;
public:
    Grid(std::array<const size_t, dimension>&& dimensions);
    ~Grid();
    void reset();
};

class World {
public:
    using Grid = ::Grid<double, 2>;
private:
    Grid grid1, grid2;
    Grid* current_grid, * other_grid;
    double t = 0;
    double dt;
public:
    World(std::size_t grid_height, std::size_t grid_width, double dt = 0.01):
        dt(dt), grid1({grid_height, grid_width}), grid2({grid_height, grid_width})
    {
        current_grid = &grid1;
        other_grid = &grid2;
    }
    void step();
    void multi_step(unsigned N);
    const Grid& grid() const { return *current_grid; }
    inline void reset() { current_grid->reset(); }
    void synchronize();
};
