#ifndef NONOWNINGARRAY_HPP
#define NONOWNINGARRAY_HPP

#include <algorithm>
#include <cstddef>

template<class T, std::size_t N>
struct ArrayView {
    T* _data;

    ArrayView(T* data) : _data(data) {}
    T* begin() const { return _data; }
    T* end() const { return _data + N; }
    [[nodiscard]] constexpr int size() const { return N; }
    T& operator[](int i) const { return _data[i]; }
    bool operator==(const ArrayView& other) const {
        return std::equal(begin(), end(), other.begin());
    }
};

#endif //NONOWNINGARRAY_HPP
