#ifndef NONOWNINGARRAY_HPP
#define NONOWNINGARRAY_HPP

#include <cstddef>

template<class T, std::size_t N>
class ArrayView {
    T* _data;
public:
    ArrayView(T* data) : _data(data) {}
    struct iterator {
        T* ptr;
    };
    struct const_iterator {
        const T* ptr;
    };
    iterator begin() { return iterator(_data); }
    iterator end() { return iterator(_data + N); }
    const_iterator begin() const { return const_iterator(_data); }
    const_iterator end() const { return const_iterator(_data + N); }
    [[nodiscard]] constexpr int size() const { return N; }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    T* data() { return _data; }
};

#endif //NONOWNINGARRAY_HPP
