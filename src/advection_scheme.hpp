#include "scheme.hpp"
#include "grid.hpp"

class UpwindScheme: public Scheme<Grid<double, 2>, 2> {
    double c;
public:
    void step(const Grid& before, Grid& after, double t, double dt) const override;
    void multi_step(unsigned int N, Grid& before, Grid& after, double t, double dt) const override;
};
