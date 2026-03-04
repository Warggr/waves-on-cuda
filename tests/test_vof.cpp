#include <gtest/gtest.h>
#include "grid.hpp"
#include "vof/vof.hpp"


TEST(VofTest, StaticScenario) {
    using dtype = int;
    const double dt = 0.01;

    StaggeredGrid<std::allocator<double>> in({ 5, 5, 5 });
    for(const auto& idxs: in.volume_fraction.indices()){
        in.volume_fraction[idxs] = 1.0;
    }
    StaggeredGrid<std::allocator<double>> out(in.volume_fraction.shape());
    VOF<std::allocator>{}.step(in, out, 0, dt);
    for(const auto& idxs: out.volume_fraction.indices()){
        EXPECT_NEAR(out.volume_fraction[idxs], 1.0, 1e-5);
    }
}
