#include <array>
#include "scheme.hpp"
#include "grid.hpp"

constexpr int ndim = 3;
using Speed = std::array<double, ndim>;

struct StaggeredGrid {
    Grid<double, ndim> volume_fraction;
    Grid<double, ndim> u[3];

    StaggeredGrid(std::array<std::size_t, ndim> dims):
        volume_fraction(dims),
        u{
            stagger(dims, 0),
            stagger(dims, 1),
            stagger(dims, 2)
        }
    {
    };

private:
    static std::array<std::size_t, ndim> stagger(std::array<std::size_t, ndim> dims, std::size_t axis){
        std::array<std::size_t, ndim> result;
        std::copy(dims.begin(), dims.end(), result.begin());
        ++result[axis];
        return result;
    }
};

class VOF: public Scheme<StaggeredGrid, 3> {
private:
    static ::Grid<double, ndim> compute_pressure(const StaggeredGrid& before, const ::Grid<Speed, ndim>& forces);
public:
    void step(const StaggeredGrid& before, StaggeredGrid& after, double t, double dt) const override;
};
