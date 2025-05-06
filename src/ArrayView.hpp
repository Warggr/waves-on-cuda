#ifndef NONOWNINGARRAY_HPP
#define NONOWNINGARRAY_HPP

#include <cstddef>

template<class T, std::size_t N>
class ArrayView {
    T* _data;
public:
    ArrayView(T* data) : _data(data) {}
    T* begin() { return iterator(_data); }
    T* end() { return iterator(_data + N); }
    const T* begin() const { return {_data}; }
    const T* end() const { return {_data + N}; }
    [[nodiscard]] constexpr int size() const { return N; }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
};

#endif //NONOWNINGARRAY_HPP
