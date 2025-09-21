#ifndef NONOWNINGARRAY_HPP
#define NONOWNINGARRAY_HPP

#include <cstddef>

template<class T, std::size_t N>
struct ArrayView {
    T* _data;

    ArrayView(T* data) : _data(data) {}
    T* begin() const { return _data; }
    T* end() const { return _data + N; }
    [[nodiscard]] constexpr int size() const { return N; }
    T& operator[](int i) const { return _data[i]; }
};

#endif //NONOWNINGARRAY_HPP
