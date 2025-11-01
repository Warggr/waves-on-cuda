#pragma once

#include "ArrayView.hpp"
#include <functional>
#include <numeric>

template<class dtype, size_t dimension>
class GridViewIterator;

template<size_t dimension>
struct ShapeIterator {
    const ArrayView<const size_t, dimension>& _size;
    std::array<size_t, dimension> _coords;
    bool finished = false;

    ShapeIterator(const ArrayView<const size_t, dimension>& size): _size(size) {
        for(unsigned i = 0; i < dimension; i++) _coords[i] = 0;
    }
    void operator++() {
        for(unsigned i = 0; i < dimension; i++){
            _coords[i]++;
            if(_coords[i] == _size[i]) {
                _coords[i] = 0;
            } else {
                return;
            }
        }
        finished = true;
    }
    const std::array<size_t, dimension> operator*() const { return _coords; }
    bool operator!=(const ShapeIterator& other){
        return other.finished == this->finished && other._coords == this->_coords;
    }
};

template<size_t dimension>
struct Shape {
    const ArrayView<const size_t, dimension>& shape;
    ShapeIterator<dimension> begin() const {
        return ShapeIterator(shape);
    }
    ShapeIterator<dimension> end() const {
        ShapeIterator<dimension> result(shape);
        result.finished = true;
    return result;
    }
};

template<class dtype, size_t dimension>
class GridView {
protected:
    dtype* _data;
    const ArrayView<const size_t, dimension> _grid_size_view;
    GridView(dtype* data, const ArrayView<const size_t, dimension>& grid_size): _data(data), _grid_size_view(grid_size) {};
public:
    GridView(dtype* data, const std::array<const size_t, dimension>& grid_size):
        GridView(data, ArrayView<const size_t, dimension>{ grid_size.data() }) {}

    std::size_t size() const {
        return std::reduce(
            shape().begin(), shape().end(),
            1,
            std::multiplies<dtype>{}
        );
    }
    const ArrayView<const size_t, dimension>& shape() const { return _grid_size_view; }
    Shape<dimension> indices() const { return Shape(_grid_size_view); }

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
    GridView(dtype* data, const ArrayView<const size_t, 1>& grid_size): _data(data), _size(grid_size[0]) {};
public:
    dtype* begin() const { return _data; }
    dtype* end() const { return _data + _size; }
    std::size_t size() const { return _size; }
    dtype& operator[] (int i) {
        return _data[i];
    }
    const dtype& operator[] (int i) const {
        return _data[i];
    }

    friend class GridView<dtype, 2>;
};

template<class dtype, size_t dimension>
struct GridViewIterator: public GridView<dtype, dimension> {
    GridViewIterator(dtype* data, const ArrayView<const size_t, dimension>& grid_size): GridView<dtype, dimension>(data, grid_size) {};
    void operator++() { this->_data += this->size(); }
    bool operator!=(const GridViewIterator& other) const { return this->_data != other._data; }
    // to compare with end()
    bool operator!=(const dtype* other) const { return this->_data != other; }
    GridView<dtype, dimension> operator*() const { return *this; }
};

template<class dtype, size_t dimension>
GridViewIterator<dtype, dimension-1> GridView<dtype, dimension>::begin() const {
    return {_data, _grid_size_view._data + 1 };
}

template<class dtype, size_t dimension>
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
