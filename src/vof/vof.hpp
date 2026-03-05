#include "grid.hpp"
#include "scheme.hpp"
#include <array>
#include <memory>

constexpr int ndim = 3;
using Speed = std::array<double, ndim>;

template<typename allocator = CUDAAllocator<double>>
struct StaggeredGrid {
    Grid<double, ndim, allocator> volume_fraction;
    Grid<double, ndim, allocator> u[3];

    StaggeredGrid(std::array<std::size_t, ndim> dims)
        : volume_fraction(dims),
          u{stagger(dims, 0), stagger(dims, 1), stagger(dims, 2)} {};

private:
    static std::array<std::size_t, ndim>
    stagger(std::array<std::size_t, ndim> dims, std::size_t axis) {
        std::array<std::size_t, ndim> result;
        std::copy(dims.begin(), dims.end(), result.begin());
        ++result[axis];
        return result;
    }
};

template<template<typename> class allocator = CUDAAllocator>
class VOF: public Scheme<StaggeredGrid<allocator<double>>, 3> {
private:
    template<typename dtype>
    using _Grid = Grid<dtype, 3, allocator<dtype>>;
    using StaggeredGrid = StaggeredGrid<allocator<double>>;
    static _Grid<double> compute_pressure(const _Grid<double>& volume_fraction,
                                          const _Grid<Speed>& u_trans,
                                          std::array<double, 3> dx);
    static _Grid<Speed> compute_transport_velocity(const StaggeredGrid& u,
                                                   _Grid<Speed> forces,
                                                   std::array<double, 3> dx);

public:
    void step(const StaggeredGrid& before, StaggeredGrid& after, double t,
              double dt) const override;
};
