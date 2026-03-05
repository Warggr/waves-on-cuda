#pragma once

#include <cstddef>
#include <utility>

template<typename GridType, unsigned int dim>
class Scheme {
public:
    using Grid = GridType;
    virtual void step(const GridType& before, GridType& after, double t,
                      double dt) const = 0;
    virtual void multi_step(unsigned int N, GridType& before, GridType& after,
                            double t, double dt) const {
        GridType *front = &before, *back = &after;
        for(int i = 0; i < N; i++) {
            step(*front, *back, t, dt);
            std::swap(front, back);
            t += dt;
        }
    }
};

template<typename Grid, unsigned int ndim>
class World {
public:
    Grid grid1, grid2;
    Grid *current_grid, *other_grid;
    double t = 0;
    double dt;

public:
    World(std::array<std::size_t, ndim> dims, double dt = 0.01)
        : grid1(dims), grid2(dims), dt(dt) {
        current_grid = &grid1;
        other_grid = &grid2;
    }
    void step(const Scheme<Grid, ndim>& scheme) {
        scheme.step(*current_grid, *other_grid, t, dt);
        std::swap(current_grid, other_grid);
        t += dt;
    }
    void multi_step(unsigned N, const Scheme<Grid, ndim>& scheme) {
        scheme.multi_step(N, *current_grid, *other_grid, t, dt);
        t += N * dt;
    }
    const Grid& grid() const {
        return *current_grid;
    }
    inline void reset(const Grid& original_grid) {
        *current_grid = original_grid;
    }
};

void synchronize();
