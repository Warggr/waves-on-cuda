#include "grid.hpp"
#include "vof/vof.hpp"
#include <gtest/gtest.h>

template<typename dtype>
#ifdef NO_CUDA
using Allocator = std::allocator<dtype>;
#else
using Allocator = CUDAAllocator<dtype>;
#endif

TEST(VofTest, StaticScenario) {
    using dtype = int;
    const double dt = 0.01;

    StaggeredGrid<Allocator<double>> in({5, 5, 5});
    for(const auto& idxs: in.volume_fraction.indices()) {
        in.volume_fraction[idxs] = 1.0;
    }
    StaggeredGrid<Allocator<double>> out(in.volume_fraction.shape());
    VOF<Allocator>{}.step(in, out, 0, dt);
    for(const auto& idxs: out.volume_fraction.indices()) {
        EXPECT_NEAR(out.volume_fraction[idxs], 1.0, 1e-5);
    }
}
