#ifndef NONOWNINGARRAY_HPP
#define NONOWNINGARRAY_HPP

#include <cstddef>

template<class T>
class ArrayIterator {
    T* ptr;
public:
    ArrayIterator(T* ptr) : ptr(ptr) {}
    void operator++() { ptr++; }
    bool operator!=(const ArrayIterator& other) const { return ptr != other.ptr; }
    T& operator*() const { return *ptr; }
};

template<class T, std::size_t N>
class ArrayView {
    T* _data;
public:
    ArrayView(T* data) : _data(data) {}
    ArrayIterator<T> begin() { return iterator(_data); }
    ArrayIterator<T> end() { return iterator(_data + N); }
    ArrayIterator<const T> begin() const { return {_data}; }
    ArrayIterator<const T> end() const { return {_data + N}; }
    [[nodiscard]] constexpr int size() const { return N; }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    T* data() { return _data; }
};

#endif //NONOWNINGARRAY_HPP
