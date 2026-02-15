#pragma once

#include "ArrayView.hpp"
#include <functional>
#include <numeric>
#include <cassert>

template<class dtype, size_t dimension>
class GridViewIterator;

template<size_t dimension>
struct ShapeIterator {
    const ArrayView<size_t, dimension>& _size;
    std::array<size_t, dimension> _coords;
    bool finished = false;

    ShapeIterator(const ArrayView<size_t, dimension>& size): _size(size) {
        for(unsigned i = 0; i < dimension; i++) _coords[i] = 0;
    }
    void operator++() {
        for(unsigned i = 0; i < dimension; i++){
            unsigned dim_i = dimension - 1 - i; // Start iterating with the lowest dimension,
            // for better locality
            _coords[dim_i]++;
            if(_coords[dim_i] == _size[dim_i]) {
                _coords[dim_i] = 0;
            } else {
                return;
            }
        }
        finished = true;
    }
    const std::array<size_t, dimension> operator*() const { return _coords; }
    bool operator!=(const ShapeIterator& other){
        return other.finished != this->finished || other._coords != this->_coords;
    }
};

template<size_t dimension>
struct Shape {
    const ArrayView<size_t, dimension>& shape;
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
    const ArrayView<size_t, dimension> _grid_size_view;
    GridView(dtype* data, const ArrayView<size_t, dimension>& grid_size): _data(data), _grid_size_view(grid_size) {};
public:
    std::size_t idx_to_offset(std::array<std::size_t, dimension> idxs) const {
        std::size_t offset = 0;
        for(int dim = 0; dim < dimension; dim++){
            offset *= _grid_size_view[dim];
            offset += idxs[dim];
        }
        return offset;
    }

    GridView(dtype* data, const std::array<size_t, dimension>& grid_size):
        GridView(data, ArrayView<size_t, dimension>{ const_cast<size_t*>(grid_size.data()) }) {}

    void operator=(const GridView<dtype, dimension>& other) {
        assert(this->_grid_size_view == other._grid_size_view);
        std::copy(other._data, other._data + other.size(), this->_data);
    }

    std::size_t size() const {
        return std::reduce(
            shape().begin(), shape().end(),
            1,
            std::multiplies<std::size_t>{}
        );
    }
    const ArrayView<size_t, dimension>& shape() const { return _grid_size_view; }
    Shape<dimension> indices() const { return Shape(_grid_size_view); }

    dtype* data() { return _data; }
    const dtype* data() const { return _data; }

    GridViewIterator<dtype, dimension-1> begin() const;
    const dtype* end() const { return { _data + size() }; }

    // Return a copy, because this is a View anyway (no data gets copied)
    GridView<dtype, dimension-1> operator[] (int i) const {
        const std::size_t subgrid_size = std::reduce(std::next(shape().begin()), shape().end(), 1, std::multiplies<std::size_t>{});
        return { _data + i*subgrid_size, { _grid_size_view._data + 1 } };
    }

    dtype& operator[](std::array<std::size_t, dimension> idxs) {
        return _data[idx_to_offset(idxs)];
    }
    const dtype& operator[](std::array<std::size_t, dimension> idxs) const {
        return _data[idx_to_offset(idxs)];
    }

    friend class GridView<dtype, dimension+1>;
};

template<class dtype>
class GridView<dtype, 1>{
protected:
    dtype* _data;
    std::size_t _size;
    GridView(dtype* data, const ArrayView<size_t, 1>& grid_size): _data(data), _size(grid_size[0]) {};
public:
    dtype* begin() const { return _data; }
    dtype* data() const { return _data; }
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
    GridViewIterator(dtype* data, const ArrayView<size_t, dimension>& grid_size): GridView<dtype, dimension>(data, grid_size) {};
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

class CUDAAllocator {
public:
    static void* calloc(std::size_t num, std::size_t size);
    static void free(void* mem);
};

template<class dtype, size_t dimension>
class Grid: public GridView<dtype, dimension> {
    const std::array<size_t, dimension> _size;
public:
    Grid(std::array<size_t, dimension> dimensions):
        _size(std::move(dimensions)),
        GridView<dtype, dimension>(nullptr, this->_size)
    {
        this->_data = static_cast<dtype*>(CUDAAllocator::calloc(this->size(), sizeof(dtype)));
    }
    ~Grid() {
        CUDAAllocator::free(this->_data);
    }
    void operator=(const Grid& other) {
        assert(this->_size == other._size);
        GridView<dtype, dimension>::operator=(other);
    }
    const std::array<size_t, dimension>& shape() const { return _size; }
};
